#include <iostream>
#include <mpi.h>
#include <time.h>
#include <chrono>
using namespace std;

// Прототипы функций
int main(int argc, char** argv);
int get_random_number(int r_min, int r_max);
int* create_random_vector(int size, int min, int max);
long long int get_partition_scalar_product(int* local_vector1, int* local_vector2, size_t vector_size);
void mpi_scalar_product(int* vector1, int* vector2, int vector_size, int rank, int master, int size, int partition, int tag);
int* create_vector_by_user_input(int size);
int input(int v_min, int v_max);
void test();

int main(int argc, char** argv) {
    setlocale(LC_CTYPE, "rus");
    int size;
    int rank;
    const int master = 0;
    int tag = 777;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Вывод интерфейса программы мастер-процессом
    if (rank == master) {
        cout << "Практическая работа №4 Распределенное программирование для систем с общей памятью с использованием основ технологии MPI. Лисовой Андрей" << endl;
        cout << std::endl;
        cout << "Осуществить тестирование алгоритма? 1 - Да, 0 - Нет." << endl;
        int do_test = input(0, 1);
        if (do_test) {
            test();
        }
    }

    int vector_size = 20;
    int* vector1 = create_random_vector(vector_size, 1, 99);
    int* vector2 = create_random_vector(vector_size, 1, 99);
    //int vector_size = 4;
    //int vector1[4] {3, 8, 2, 4};
    //int vector2[4]{ 6, 4, 1, 3 };
    //int partition = vector_size / size;

    if (size == 1 && rank == master)
        cout << "Запуск последовательного выполнения алгоритма для длины векторов: " << vector_size << endl;
    else if (size > 1 && rank == master)
        cout << "Запуск распределенного выполнения алгоритма для длины векторов = " << vector_size << " при количестве процессов = " << size <<  endl;
    mpi_scalar_product(vector1, vector2, vector_size, rank, master, size, partition, tag);

    MPI_Finalize();
    return 0;
}

// Метод генерации случайного числа
int get_random_number(int r_min, int r_max) {
    int r_number = rand() % (r_max - r_min + 1) + r_min;
    return r_number;
}

// Метод создания вектора путем заполнения его случайными числами
int* create_random_vector(int vector_size, int min, int max) {
    srand(static_cast<unsigned int>(time(NULL)));
    int* vector = new int[vector_size];
    for (int i = 0; i < vector_size; i++) {
        vector[i] = get_random_number(min, max);
    }
    return vector;
}

// Вспомогательный метод поиска скалярного произведения
long long int get_partition_scalar_product(int* local_vector1, int* local_vector2, size_t vector_size) {
    long long int scalar_product = 0;
    // cout << "Я должен пройти в цикле по части векторов начиная от 0 и до " << vector_size << endl;
    for (size_t i = 0; i < vector_size; i++) {
        scalar_product += local_vector1[i] * local_vector2[i];
        // cout << "Скалярное произведение local_vector1[" << i << "] и local_vector2[" << i << "]: " << local_vector1[i] * local_vector2[i] << endl;
    }
    cout << "Скалярное произведение данных локальных векторов равно " << scalar_product << endl;
    return scalar_product;
}

// Распределенная с помощью MPI версия алгоритма
void mpi_scalar_product(int* vector1, int* vector2, int vector_size, int rank, int master, int size, int partition, int tag) {
    int* local_vector1 = (int*) malloc (partition * sizeof(int));
    int* local_vector2 = (int*) malloc (partition * sizeof(int));
    long long local_scalar_product;
    long long global_scalar_product;

    if (rank == master) {
        int slave;
        int master_partition = 0;
        for (slave = 1; slave < size; ++slave) {
            MPI_Send(&vector1[(slave - 1) * partition], partition, MPI_INT, slave, tag, MPI_COMM_WORLD);
            MPI_Send(&vector2[(slave - 1) * partition], partition, MPI_INT, slave, tag, MPI_COMM_WORLD);
            master_partition = ((slave - 1) * partition) + partition;
            cout << "Мастер-процесс передал " << partition << " элементов на обработку процессу P" << slave << endl;
        }
        cout << "Мастер-процесс оставил " << vector_size - master_partition << " элементов себе на обработку" << endl;
        local_scalar_product = 0;
        for (master_partition; master_partition < vector_size; master_partition++) {
            local_scalar_product += vector1[master_partition] * vector2[master_partition];
        }
        cout << "Мастер-процесс для своей части вычислил скалярное произведение = " << local_scalar_product << endl;
    }
    else {
        MPI_Recv(local_vector1, partition, MPI_INT, master, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        cout << "Процесс P" << rank << " получил " << partition << " элементов первого вектора на обработку" << endl;
        MPI_Recv(local_vector2, partition, MPI_INT, master, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        cout << "Процесс P" << rank << " получил " << partition << " элементов второго вектора на обработку" << endl;
        local_scalar_product = get_partition_scalar_product(local_vector1, local_vector2, partition);
        cout << "Процесс P" << rank << " для своей части вычислил скалярное произведение = " << local_scalar_product << endl;
    }

    MPI_Reduce(&local_scalar_product, &global_scalar_product, 1, MPI_LONG_LONG, MPI_SUM, master, MPI_COMM_WORLD);

    if (rank == master) {
        cout << "Скалярное произведение векторов: " << global_scalar_product << endl;
    }
}

// Метод ввода вектора с клавиатуры
int* create_vector_by_user_input(int vector_size) {
    bool flag = false;
    int* vector = new int[vector_size];
    for (int i = 0; i < vector_size; i++) {
        vector[i] = input(1, 10);
    }
    return vector;
}

// Метод фильтрации ввода
int input(int v_min, int v_max) {
    cout << ">>> ";
    int num;
    while (!(cin >> num) || (num < v_min || num > v_max)) {
        cin.clear();
        while (cin.get() != '\n');
        cout << "Вы ввели недопустимое значение!" << endl;
        cout << "Повторите ввод." << endl;
        cout << ">>> ";
    }
    return num;
}

// Метод тестирования алгоритма
void test() {
    cout << endl;
    cout << "Запуск тестирования алгоритма" << endl;
    int size = 10;
    cout << "Введите построчно элементы вектора А:" << endl;
    int* vector1 = create_vector_by_user_input(size);
    cout << "Введите построчно элементы вектора В:" << endl;
    int* vector2 = create_vector_by_user_input(size);
    long long int scalar_product = get_partition_scalar_product(vector1, vector2, size);
    cout << "Скалярное произведение данных векторов равно " << scalar_product << endl;
    cout << "Тестирование успешно завершено" << endl;
    cout << endl;
}