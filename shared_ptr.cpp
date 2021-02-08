#include <iostream>

template <typename T1> struct def_deletor {
  void operator()(T1 *ptr) { delete ptr; }
};
template <typename T2> struct def_deletor_array {
  void operator()(T2 *ptr) { delete[] ptr; }
};

struct count_ref {
  size_t count_{};

  count_ref() noexcept : count_{1} {}
  virtual void destroy() = 0;
  virtual ~count_ref(){};
};

template <typename U, typename Deletor> struct delete_block : count_ref {
  U *data_ref{};
  Deletor del{};

  delete_block() noexcept = default;
  delete_block(U *ptr, Deletor d) : count_ref(), data_ref{ptr}, del{d} {}

  void destroy() override { del(data_ref); }
};

template <typename T> class shared_ptr {
public:
  shared_ptr() noexcept = default;

  template <typename Y>
  explicit shared_ptr(Y *ptr) noexcept
      : data{ptr}, count{new delete_block<Y, def_deletor<Y>>(
                       ptr, def_deletor<Y>())} {}

  template <typename Y, typename Deletor>
  shared_ptr(Y *ptr, Deletor del)
      : data{ptr}, count{new delete_block<Y, Deletor>(ptr, del)} {}

  shared_ptr(const shared_ptr &rhs) noexcept
      : data{rhs.data}, count{rhs.count} {
    increm_count();
  }

  shared_ptr &operator=(const shared_ptr &rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    decrem_count();
    data = rhs.data;
    count = rhs.count;
    increm_count();

    return *this;
  }

  shared_ptr(shared_ptr &&rhs) noexcept {
    decrem_count();
    data = std::move(rhs.data);
    count = std::move(rhs.count);
    rhs.data = nullptr;
    rhs.count = nullptr;
  }

  shared_ptr &operator=(shared_ptr &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    decrem_count();
    data = std::move(rhs.data);
    count = std::move(rhs.count);
    rhs.data = nullptr;
    rhs.count = nullptr;

    return *this;
  }

  ~shared_ptr() noexcept { decrem_count(); }

  void increm_count() {
    if (data != nullptr) {
      ++(count->count_);
    }
  }

  void decrem_count() {
    if (--(count->count_) == 0 && data != nullptr) {
      count->destroy();
      delete count;
    }
  }

  void reset() noexcept {
    decrem_count();
    data = nullptr;
    count = nullptr;
  }

  template <typename Y> void reset(Y *ptr) {
    decrem_count();
    data = ptr;
    count = new delete_block<Y, def_deletor<Y>>(ptr, def_deletor<Y>());
  }

  template <typename Y, typename Deletor> void reset(Y *ptr, Deletor d) {
    decrem_count();
    data = ptr;
    count = new delete_block<Y, Deletor>(ptr, d);
  }

  size_t use_count() const { return count->count_; }
  T *get() const { return data; }
  explicit operator bool() const { return get() != nullptr; }
  T *operator->() const { return data; }
  T &operator*() const { return *data; }

private:
  T *data{};
  count_ref *count{};
};

template <typename T> class shared_ptr<T[]> {
public:
  shared_ptr() noexcept = default;

  template <typename Y>
  explicit shared_ptr(Y *ptr) noexcept
      : data{ptr}, count{new delete_block<Y, def_deletor_array<Y>>(
                       ptr, def_deletor_array<Y>())} {}

  template <typename Y, typename Deletor>
  shared_ptr(Y *ptr, Deletor del)
      : data{ptr}, count{new delete_block<Y, Deletor>(ptr, del)} {}

  shared_ptr(const shared_ptr &rhs) noexcept
      : data{rhs.data}, count{rhs.count} {
    increm_count();
  }

  shared_ptr &operator=(const shared_ptr &rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    decrem_count();
    data = rhs.data;
    count = rhs.count;
    increm_count();

    return *this;
  }

  shared_ptr(shared_ptr &&rhs) noexcept {
    decrem_count();
    data = std::move(rhs.data);
    count = std::move(rhs.count);
    rhs.data = nullptr;
    rhs.count = nullptr;
  }

  shared_ptr &operator=(shared_ptr &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    decrem_count();
    data = std::move(rhs.data);
    count = std::move(rhs.count);
    rhs.data = nullptr;
    rhs.count = nullptr;

    return *this;
  }

  ~shared_ptr() noexcept { decrem_count(); }

  void increm_count() {
    if (data != nullptr) {
      ++(count->count_);
    }
  }

  void decrem_count() {
    if (--(count->count_) == 0 && data != nullptr) {
      count->destroy();
      delete count;
    }
  }

  void reset() noexcept {
    decrem_count();
    data = nullptr;
    count = nullptr;
  }

  template <typename Y> void reset(Y *ptr) {
    decrem_count();
    data = ptr;
    count =
        new delete_block<Y, def_deletor_array<Y>>(ptr, def_deletor_array<Y>());
  }

  template <typename Y, typename Deletor> void reset(Y *ptr, Deletor d) {
    decrem_count();
    data = ptr;
    count = new delete_block<Y, Deletor>(ptr, d);
  }

  size_t use_count() const { return count->count_; }
  T *get() const { return data; }
  explicit operator bool() const { return get() != nullptr; }
  T *operator->() const { return data; }
  T &operator*() const { return *data; }
  T &operator[](size_t index) const { return get()[index]; }

private:
  T *data{};
  count_ref *count{};
};

int main() {
  shared_ptr<int> ptr1{new int{100}};
  shared_ptr<int[]> ptr2{new int[200]{7, 8, 9}};

  shared_ptr<int> ptr3{ptr1};
  auto ptr4 = ptr2;
  std::cout << "ptr4 count ref " << ptr4.use_count() << "\n";
  std::cout << "ptr3 count ref " << ptr3.use_count() << "\n";
  std::cout << "ptr1 count ref " << ptr1.use_count() << "\n";

  ptr1.reset(new int{50});
  std::cout << "ptr3 count ref " << ptr3.use_count() << "\n";
  std::cout << "ptr1 count ref " << ptr1.use_count() << "\n";

  std::cout << "ptr1 get() " << *ptr1.get() << '\n'
            << "ptr3 get() " << *ptr3.get() << "\n"
            << "ptr2[1] " << ptr2[1] << "\n"
            << "ptr4[1] " << ptr4[1] << "\n";

  auto file1 = fopen("text.txt", "r");
  auto close_file = [](FILE *file) { fclose(file); };
  shared_ptr<FILE> ptr25(file1, close_file); // use ~Shared_ptr() {fclose(ptr);}

  return 0;
}
