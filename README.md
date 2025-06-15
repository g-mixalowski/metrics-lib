# Библиотека для сбора и записи метрик на C++
## Обзор
Библиотека представляет собой набор классов для сбора и записи метрик в формате, приближенном к совместимому с OpenMetrics (Prometheus). Ключевые особенности:
* **Потокобезопасность:** Атомарные операции и блокировки для многопоточных сред.
* **Гибкость:** Поддержка разных типов метрик и их комбинаций.
* **Простота пользования:** Интуитивно понятный API.
* **Производительность:** Количество операций в каждом конкретном use-case приближено к минимальному.
## Основные компоненты
### 1. Базовый интерфейс `Metric`
```cpp
class Metric {
public:
    virtual ~Metric() = default;
    virtual std::string name() const = 0;
    virtual std::string value_as_str() const = 0;
    virtual void reset() = 0;
};
```
Все метрики реализуют этот интерфейс, обесечивая:
* `name()` - получение имени метрики;
* `value_as_str()` - получение значения в виде строки;
* `reset()` - сброс состояния метрики в дефолтное положение.

### 2. Метрики различных типов
#### 2.1. `Counter`

```cpp
template <typename N = uint64_t, typename A = std::atomic<N>>
class Counter : public Metric {
public:
    Counter(std::string name);
    N inc();
    N inc_by(N v);
    N get() const;
    // реализация интерфейса Metric
};
```
* **Назначение:** Монотонно возрастающие величины (накапливающиеся).
* **Методы:**
    * `inc()` - увеличить на 1;
    * `inc_by(v)` - увеличить на `v`;
    * `get()` - получить текущее значение.
#### 2.2. `Gauge`
```cpp
template <typename N = uint64_t, typename A = std::atomic<N>>
class Gauge : public Metric {
public:
    Gauge(std::string name);
    N inc();
    N inc_by(N v);
    N dec();
    N dec_by(N v);
    N set(N v);
    N get() const;
    // реализация интерфейса Metric
};
```
* **Назначение:** Значения, которые могут увеличиваться или уменьшаться.
* **Методы:**
    * `set(v)` - установить значение `v`;
    * `inc()/dec()` - уменьшить/увеличить значение на 1;
    * `inc_by(v)/dec_by(v)` - уменьшить/увеличить значение на `v`.
#### 2.3 `Histogram`
```cpp
class Histogram : public Metric {
public:
    Histogram(std::string name, const std::vector<double>& buckets);
    void observe(double value);
    Snapshot get() const;
    // реализация интерфейса Metric
};
```
* **Назначение:** Значения, распределённые в некотором диапазоне.
* **Методы:**
    * `observe(value)` - зафиксировать наблюдение;
    * `get()` - получить снэпшот текущего состояния.
* **Генераторы бакетов:**
    * `exponential_buckets(start, factor, length)` - бакеты с экспоненциально возрастающей длиной;
    * `linear_buckets(start, width, lenth)` - бакеты фиксированной длины;
    * `exponential_buckets_range(min, max, length)` - бакеты с экспоненциально возрастающей в заданном диапазоне длиной.
#### 2.4 `Info`
```cpp
template <typename S = std::string>
class Info : public Metric {
public:
    Info(std::string name, std::vector<std::pair<S, S>> labels);
    // реализация интерфейса Metric
};
```
* **Назначение:** статическая информация об окружении;
* **Инициализация:** пары строковых значений вида "ключ: значение".
### 3. `MetricsCollector`
```cpp
class MetricsCollector {
public:
    void register_metric(std::shared_ptr<Metric> metric);
    void flush(const std::string& filename);
};
```
* **Назначение:** Управление множеством метрик и их запись.
* **Методы:**
    * `register_metric(metric)` - добавление метрики;
    * `flush(filename)` - записать метрики в файл.
## Сборка и запуск.
```bash
mkdir && cd build
cmake -DCMAKE_PREFIX_INSTALL=/usr/local .. # путь для установки библиотеки
cmake --build . && cmake --install .
```
Для сборки примеров из папки `examples`, необходимо использовать флаг `-DMETRICS_BUILD_EXAMPLES=ON`. Запуск примеров:
```bash
./examples/basic_example
./examples/system_monitor
```
## Заключение
Весь код вышеописанной библиотеки, а также данную краткую документацию написал Михаловский Михаил Михайлович, студент программы бакалавриата "Прикладная математика и информатика" Школы Физики, Информатики и Технологий НИУ ВШЭ (Санкт-Петербург) в качестве тестового задания для компании VK.
