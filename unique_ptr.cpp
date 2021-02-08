#include <iostream>
#include <memory>

template <typename T, typename Deletor = std::default_delete<T>>
class unique_ptr {
public:
  unique_ptr() noexcept = default;
  explicit unique_ptr(T *ptr) noexcept : data{ptr} {}
  unique_ptr(T *ptr, Deletor d) : data{ptr}, del{d} {}

  unique_ptr(const unique_ptr &rhs) = delete;
  unique_ptr &operator=(const unique_ptr &rhs) = delete;

  unique_ptr(unique_ptr &&rhs) noexcept : data{std::move(rhs.data)} {
    rhs.data = nullptr;
  }

  unique_ptr &operator=(unique_ptr &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    data = std::move(rhs.data);
    rhs.data = nullptr;

    return *this;
  }

  ~unique_ptr() {
    if (data != nullptr) {
      del(data);
    }
  }

  T *release() {
    T *tmp_ptr = std::move(data);
    data = nullptr;
    return tmp_ptr;
  }

  void reset(T *ptr) {
    T *tmp_ptr = std::move(data);
    data = ptr;
    if (tmp_ptr != nullptr) {
      del(tmp_ptr);
    }
  }

  T *get() const { return data; }
  explicit operator bool() const { return get() != nullptr; }
  T *operator->() const { return data; }
  T &operator*() const { return *data; }

private:
  T *data{};
  Deletor del;
};

template <typename T, typename Deletor> class unique_ptr<T[], Deletor> {
public:
  unique_ptr() = default;
  explicit unique_ptr(T *ptr) : data{ptr} {}
  unique_ptr(T *ptr, Deletor d) : data{ptr}, del{d} {}

  unique_ptr(const unique_ptr &rhs) = delete;
  unique_ptr &operator=(const unique_ptr &rhs) = delete;

  unique_ptr(unique_ptr &&rhs) noexcept : data{std::move(rhs.data)} {
    rhs.data = nullptr;
  }

  unique_ptr &operator=(unique_ptr &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    data = std::move(rhs.data);
    rhs.data = nullptr;

    return *this;
  }

  ~unique_ptr() {
    if (data != nullptr) {
      del(data);
    }
  }

  T *release() {
    T *tmp_ptr = std::move(data);
    data = nullptr;
    return tmp_ptr;
  }

  void reset(T *ptr) {
    T *tmp_ptr = std::move(data);
    data = ptr;
    if (tmp_ptr != nullptr) {
      del(tmp_ptr);
    }
  }

  T *get() const { return data; }
  explicit operator bool() const { return get() != nullptr; }
  T *operator->() const { return data; }
  T &operator*() const { return *data; }
  T &operator[](size_t index) const { return get()[index]; }

private:
  T *data{};
  Deletor del;
};

int main() {
  int *arr = new int[10]{4, 5, 6, 7};
  unique_ptr<int[]> smart_arr(arr);
  std::cout << smart_arr[2] << "\n";

  unique_ptr<int> ptr1(new int{155});
  std::cout << *ptr1.get() << "\n";

  auto file = fopen("test.txt", "r");
  auto close_file = [](FILE *file) { fclose(file); };
  unique_ptr<FILE, decltype(close_file)> ptr_file(file, close_file);

  // unique_ptr<int> ptr_copy{ptr1}; // compiler error
  unique_ptr<int> ptr_copy{std::move(ptr1)};

  unique_ptr<int> ptr_{new int{5}};
  auto ptr_own = ptr_.release();
  std::cout << *ptr_own << ' ' << ptr_.get() << ' ' << "\n";
  delete ptr_own;

  unique_ptr<int> res_ptr(new int{27});
  std::cout << *res_ptr.get() << "\n";
  res_ptr.reset(new int{123});
  std::cout << *res_ptr.get() << "\n";

  unique_ptr<int[]> res_ptr_arr(new int[27]{});
  res_ptr_arr.reset(new int[123]);
  res_ptr_arr[120] = 300;
  std::cout << res_ptr_arr[120] << "\n";

  return 0;
}
