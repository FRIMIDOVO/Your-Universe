#include "barnes_hut.h"
#include <algorithm>
#include <cmath>

void BarnesHutTree::build(
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& mass,
    const PhysicsConfig& phys_config
)
{
    clear();
    if (posX.empty()) return;

    int num_particles = static_cast<int>(posX.size());

    // Расширяем индексную ленту
    particle_indices_.resize(num_particles);
    for (int i = 0; i < num_particles; ++i) {
        particle_indices_[i] = i;
    }
    
    // Используем буфер
    temp_partition_.resize(num_particles);

    // Находим границы мира
    float minX = posX[0], maxX = posX[0];
    float minY = posY[0], maxY = posY[0];
    float minZ = posZ[0], maxZ = posZ[0];

    for (int i = 1; i < num_particles; ++i) {
        if (posX[i] < minX) minX = posX[i]; if (posX[i] > maxX) maxX = posX[i];
        if (posY[i] < minY) minY = posY[i]; if (posY[i] > maxY) maxY = posY[i];
        if (posZ[i] < minZ) minZ = posZ[i]; if (posZ[i] > maxZ) maxZ = posZ[i];
    }

    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float max_edge = std::max({sizeX, sizeY, sizeZ});

    Box root_box;
    root_box.centerX = minX + sizeX * 0.5f;
    root_box.centerY = minY + sizeY * 0.5f;
    root_box.centerZ = minZ + sizeZ * 0.5f;
    root_box.half_size = (max_edge * 0.5f) + 0.001f; 

    // Резервируем с запасом
    nodes_.reserve(num_particles * 2 + 8);

    // Создаем корень
    OctreeNode root_node;
    root_node.box = root_box;
    root_node.first_particle_idx = 0;
    root_node.particle_count = num_particles;
    root_node.first_child_idx = -1; 
    root_node.next = -1;    // корень — финал обхода
    
    nodes_.push_back(root_node);
    build_stack_.push_back({0, 0, num_particles});

    // Пока есть таски, исполняем
    while (!build_stack_.empty()) {
        BuildTask task = build_stack_.back();
        build_stack_.pop_back();

        int count = task.end - task.start;

        // Условие остановки деления куба
        if (count <= LEAF_CAPACITY) continue;

        Box parent_box = nodes_[task.node_index].box;

        // Доп проверка размера в сравнении со сглаживанием гравитации
        if (parent_box.half_size * 2.0f < phys_config.softening) continue;

        auto octant_counts = sort_into_octants(task.start, task.end, parent_box, posX, posY, posZ);

        // Создаём детей
        int first_child_idx = static_cast<int>(nodes_.size());
        nodes_[task.node_index].first_child_idx = first_child_idx;
        
        int current_offset = task.start;
        for (int i = 0; i < 8; ++i) {
            OctreeNode child;
            child.box = parent_box.get_child_box(i);
            child.first_child_idx = -1;
            child.first_particle_idx = current_offset;
            child.particle_count = octant_counts[i];
            
            nodes_.push_back(child);
            
            // Если в октанте есть частицы — отправляем его в стек на деление дальше
            if (octant_counts[i] > 0) {
                build_stack_.push_back({first_child_idx + i, current_offset, current_offset + octant_counts[i]});
            }
            current_offset += octant_counts[i];
        }

        // Первые 7 братьев указывают на следующего за ними брата
        for (int i = 0; i < 7; ++i) {
            nodes_[first_child_idx + i].next = first_child_idx + i + 1;
        }
        // Самый последний ребенок наследует next своего родителя (выход наверх)
        nodes_[first_child_idx + 7].next = nodes_[task.node_index].next;
    }

    // Распределение масс
    compute_mass_distribution(posX, posY, posZ, mass);
}

void BarnesHutTree::compute_all_accelerations(
    std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& mass,
    const PhysicsConfig& phys_config
) const
{
    int num_particles = static_cast<int>(posX.size());
    
    // Волшебство нескольких потоков
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < num_particles; ++i) {
        // Данные текущей частицы, на которую ищем влияние гравитации
        float px = posX[i];
        float py = posY[i];
        float pz = posZ[i];
        float p_mass = mass[i];

        // Накопители результирующего ускорения
        float ax = 0.0f;
        float ay = 0.0f;
        float az = 0.0f;

        // Начинаем обход с корня (индекс 0)
        int curr_idx = 0;

        while (curr_idx != -1) {
            const OctreeNode& node = nodes_[curr_idx];

            // Пропускаем пустые кубы
            if (node.is_empty()) { curr_idx = node.next; continue; }

            // Вектор расстояния до центра масс текущего узла
            float dx = node.comX - px;
            float dy = node.comY - py;
            float dz = node.comZ - pz;

            // Считаем квадрат расстояния
            float dist_sqr = dx * dx + dy * dy + dz * dz + phys_config.softening_sq;
            float dist = std::sqrt(dist_sqr);

            // Физический размер текущего узла пространства
            float s = node.box.half_size * 2.0f;

            // Критерий аппроксимации Барнса-Хата (узел далеко ИЛИ это лист)
            if (node.is_leaf() || (s / dist < phys_config.theta)) {

                // Если нод лист (без детей)
                if (node.is_leaf()) {

                    // Честно считаем гравитацию от всех его частиц (макс 8 штук)
                    for (int p = 0; p < node.particle_count; ++p) {
                        int p_idx = particle_indices_[node.first_particle_idx + p];
                        if (p_idx == i) continue; // Саму себя частица не притягивает

                        float pdx = posX[p_idx] - px;
                        float pdy = posY[p_idx] - py;
                        float pdz = posZ[p_idx] - pz;
                        float p_dist_sqr = pdx * pdx + pdy * pdy + pdz * pdz + phys_config.softening_sq;
                        float p_dist = std::sqrt(p_dist_sqr);

                        // Ускорение
                        float acc_mag = (phys_config.G * mass[p_idx]) / (p_dist_sqr * p_dist);
                        ax += pdx * acc_mag;
                        ay += pdy * acc_mag;
                        az += pdz * acc_mag;

                    }

                } else {    // Если внутренний узел далеко

                    // Притягиваемся ко всей его массе разом за один шаг
                    float acc_mag = (phys_config.G * node.total_mass) / (dist_sqr * dist);
                    ax += dx * acc_mag;
                    ay += dy * acc_mag;
                    az += dz * acc_mag;

                }

                // Пропускаем детей
                curr_idx = node.next;

            } else {
                // Узел слишком близко, спускаемся на один уровень глубже к первому ребенку
                curr_idx = node.first_child_idx;
            }
        }

        // Записываем финальные накопленные ускорения
        accX[i] += ax;
        accY[i] += ay;
        accZ[i] += az;

    }
}

void BarnesHutTree::clear()
{
    nodes_.clear();
    build_stack_.clear();
    particle_indices_.clear();
}

std::array<int, 8> BarnesHutTree::sort_into_octants(
    int start, int end, 
    const Box& parent_box, 
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ   
)
{
    int octant_counts[8] = {0};
    for (int i = start; i < end; ++i) {
        int p_idx = particle_indices_[i];
        int oct = parent_box.get_octant(posX[p_idx], posY[p_idx], posZ[p_idx]);
        octant_counts[oct]++;
    }

    int octant_offsets[8];
    octant_offsets[0] = start;
    for (int i = 1; i < 8; ++i) octant_offsets[i] = octant_offsets[i - 1] + octant_counts[i - 1];

    int write_pos[8];
    for (int i = 0; i < 8; ++i) write_pos[i] = octant_offsets[i];

    for (int i = start; i < end; ++i) {
        int p_idx = particle_indices_[i];
        int oct = parent_box.get_octant(posX[p_idx], posY[p_idx], posZ[p_idx]);
        temp_partition_[write_pos[oct]++] = p_idx;
    }

    for (int i = start; i < end; ++i) {
        particle_indices_[i] = temp_partition_[i];
    }
    
    std::array<int, 8> result;
    for (int i = 0; i < 8; ++i) result[i] = octant_counts[i];
    return result;
}

void BarnesHutTree::compute_mass_distribution(
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& mass
)
{
    // Перебираем, начиная с последнего элемента до корня
    for (int i = static_cast<int>(nodes_.size()) - 1; i >= 0; --i) {
        OctreeNode& node = nodes_[i];

        // Сбрасываем старые значения
        node.total_mass = 0.0f;
        node.comX = 0.0f; node.comY = 0.0f; node.comZ = 0.0f;

        // Если узел лист дерева (без детей)
        if (node.is_leaf()) {

            // Мы просто линейно перебираем все его частицы
            for (int p = 0; p < node.particle_count; ++p) {
                // Достаем индекс частицы из нашей отсортированной ленты
                int p_idx = particle_indices_[node.first_particle_idx + p];
                
                float m = mass[p_idx];
                node.total_mass += m;
                node.comX += posX[p_idx] * m;
                node.comY += posY[p_idx] * m;
                node.comZ += posZ[p_idx] * m;
            }

        } else {    // Если узел имеет разветвления

            /*
                Собираем данные с его 8 детей. Благодаря обратному циклу,
                к этому моменту все 8 детей гарантированно УЖЕ посчитаны!
            */
            for (int c = 0; c < 8; ++c) {
                int child_idx = node.first_child_idx + c;
                const auto& child = nodes_[child_idx];
                
                float ch_tm = child.total_mass;
                if (ch_tm > 0.0f) {
                    node.total_mass += ch_tm;
                    node.comX += child.comX * ch_tm;
                    node.comY += child.comY * ch_tm;
                    node.comZ += child.comZ * ch_tm;
                }
            }

        }

        // Если в узле есть масса, делим взвещенную сумму на n_tm -> получаем координаты центра масс
        float& n_tm = node.total_mass;
        if (n_tm > 0.0f) {
            node.comX /= n_tm;
            node.comY /= n_tm;
            node.comZ /= n_tm;
        }

    }
}

void BarnesHutTree::query_radius(
    std::vector<int>& out_indices,
    float px, float py, float pz, float R,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<uint8_t>& is_dark
) const 
{
    if (nodes_.empty()) return;

    float R_sq = R * R;
    int curr_idx = 0;   // с корня

    while (curr_idx != -1) {
        const OctreeNode& node = nodes_[curr_idx];

        // Пропускаем пустые узлы
        if (node.is_empty()) { curr_idx = node.next; continue; }

        // Проверяем расстояние до точки
        if (node.box.sq_dist(px, py, pz) > R_sq) { curr_idx = node.next; continue; }

        // Если пересекается, и лист
        if (node.is_leaf()) {
            // Проверяем каждую частицу внутри листа
            for (int p = 0; p < node.particle_count; ++p) {
                int p_idx = particle_indices_[node.first_particle_idx + p];

                if (!is_dark[p_idx])
                    out_indices.push_back(p_idx);

                /*
                // Проверяем расстояние
                float dx = posX[p_idx] - px;
                float dy = posY[p_idx] - py;
                float dz = posZ[p_idx] - pz;
                float dist_sq = dx * dx + dy * dy + dz * dz;
                
                // Если частица лежит внутри сферы, сохраняем её реальный индекс
                if (dist_sq <= R_sq) out_indices.push_back(p_idx);
                */
            }
        
            // Лист обработан — переходим к след. узлу
            curr_idx = node.next;

        } else {
            // Идём глубже
            curr_idx = node.first_child_idx;
        }

    }

}