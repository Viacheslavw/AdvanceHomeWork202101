#include <exception>
#include <ostream>
#include <string>
#include <type_traits>

#pragma once

// I know about "std::bad_alloc". The next class creation is for educational
// purposes :)
class ErrorMemoryAlloc : public std::exception {
public:
  ErrorMemoryAlloc(std::string error) : errorString{error} {}
  const char *what() const noexcept override { return errorString.c_str(); }
private:
  std::string errorString;
};

template <typename T> class vector {
public:
  vector(size_t size);

  vector(std::initializer_list<T> il);
  vector(T *begin, T *end);
  vector(const vector &);
  ~vector();
  vector &operator=(const vector &);

  // methods
  T *begin() { return pArray; }
  T *end() {
    if (pArray == nullptr) {
      return nullptr;
    }
    return pArray + arraySize;
  }
  void push_back(T value);
  void push_front(T value);

  template <typename... T1> void emplace_back(T1 &&... data);

  T *insert(T *pos, T value);
  T *erase(size_t pos);
  T *erase(T *pos);
  T *erase(T *begin, T *end);

  T back() const { return *(pArray + arraySize - 1); }
  T front() const { return *pArray; }
  T &operator[](size_t pos) { return *(pArray + pos); }
  const T &operator[](size_t pos) const { return *(pArray + pos); };
  void resize(size_t count);
  void reserve(size_t new_cap);
  size_t size() const noexcept { return arraySize; }
  size_t capacity() const noexcept { return arrayCapacity; }
  bool empty() const noexcept { return (arraySize == 0); }

private:
  T *pArray{};
  size_t arraySize{};
  size_t arrayCapacity{};
  bool isAbleToMemcpy();
  void destroyArray();
  constexpr static size_t MAKE_DOUBLE{2};
};
//----------------------------------------------------------------------
template <typename T>
vector<T>::vector(size_t size) : arraySize{size}, arrayCapacity{size} {
  static_assert(std::is_default_constructible<T>::value,
                "type T has no default constructor");
  if (size == 0) {
    return;
  }
  pArray = static_cast<T *>(malloc(arrayCapacity * sizeof(T)));
  for (size_t index = 0; index < arraySize; ++index) {
    new (pArray + index) T{};
  }
}
//----------------------------------------------------------------------
template <typename T>
vector<T>::vector(std::initializer_list<T> il) : arrayCapacity{il.size()} {
  if (il.size() == 0) {
    return;
  }
  pArray = static_cast<T *>(malloc(arrayCapacity * sizeof(T)));
  for (const T &v : il) {
    new (pArray + arraySize) T{v};
    ++arraySize;
  }
}
//----------------------------------------------------------------------
template <typename T> vector<T>::vector(T *begin, T *end) {
  static_assert(std::is_copy_constructible<T>::value,
                "type T has no copy constructor");
  if (begin == nullptr || end == nullptr) {
    throw std::out_of_range("incoming pointers are not valid");
  }
  if (begin == end) {
    throw std::out_of_range("incoming pointers point to the same object");
  }
  T *element{begin};
  while (element != end) {
    ++arraySize;
    ++element;
  }
  arrayCapacity = arraySize;
  pArray = static_cast<T *>(malloc(arrayCapacity * sizeof(T)));
  if (pArray == nullptr) {
    arraySize = 0;
    arrayCapacity = 0;
    throw ErrorMemoryAlloc("Error memory allocation");
  }
  element = begin;
  if (isAbleToMemcpy()) {
    memcpy(pArray, begin, arraySize * sizeof(T));
    return;
  }
  for (size_t index = 0; index < arraySize; ++index) {
    new (pArray + index) T{*begin};
    ++begin;
  }
}
//----------------------------------------------------------------------
template <typename T>
vector<T>::vector(const vector &rhs)
    : arraySize{rhs.arraySize}, arrayCapacity{rhs.arrayCapacity} {
  if (rhs.arrayCapacity == 0) {
    return;
  }
  pArray = static_cast<T *>(malloc(arrayCapacity * sizeof(T)));
  if (pArray == nullptr) {
    arraySize = 0;
    arrayCapacity = 0;
    throw ErrorMemoryAlloc("Error memory allocation");
  }
  if (isAbleToMemcpy()) {
    memcpy(pArray, rhs.pArray, arraySize * sizeof(T));
    return;
  }
  static_assert(std::is_copy_constructible<T>::value,
                "type T has no copy constructor");
  for (size_t index = 0; index < arraySize; ++index) {
    new (pArray + index) T{*(rhs.pArray + index)};
  }
}
//----------------------------------------------------------------------
template <typename T> vector<T>::~vector<T>() {
  if (arrayCapacity == 0) {
    pArray = nullptr;
    return;
  }
  destroyArray();
}
//----------------------------------------------------------------------
template <typename T> vector<T> &vector<T>::operator=(const vector &rhs) {
  if (&rhs == this) {
    return *this;
  }
  for (size_t index = 0; index < arraySize; ++index) {
    (pArray + index)->~T();
  }
  free(pArray);
  arraySize = rhs.arraySize;
  arrayCapacity = rhs.arrayCapacity;
  pArray = static_cast<T *>(malloc(arrayCapacity * sizeof(T)));
  if (pArray == nullptr) {
    arraySize = 0;
    arrayCapacity = 0;
    throw ErrorMemoryAlloc("Error memory allocation");
  }
  if (isAbleToMemcpy()) {
    memcpy(pArray, rhs.pArray, arraySize * sizeof(T));
    return *this;
  }
  static_assert(std::is_copy_constructible<T>::value,
                "type T has no copy constructor");
  for (size_t index = 0; index < arraySize; ++index) {
    new (pArray + index) T{*(rhs.pArray + index)};
  }
  return *this;
}
//----------------------------------------------------------------------
template <typename T> void vector<T>::push_back(T value) {
  if (arraySize == arrayCapacity) {
    reserve(arrayCapacity * MAKE_DOUBLE);
  }
  if (isAbleToMemcpy()) {
    memcpy(pArray + arraySize, &value, sizeof(T));
  } else {
    static_assert(std::is_copy_constructible<T>::value,
                  "type T has no copy constructor");
    new (pArray + arraySize) T{value};
  }
  ++arraySize;
}
//----------------------------------------------------------------------
template <typename T> void vector<T>::push_front(T value) {
  size_t cap{1};
  if (arrayCapacity > 0) {
    cap = arrayCapacity + arrayCapacity * (arraySize / arrayCapacity);
  }
  T *tempArray = static_cast<T *>(malloc(cap * sizeof(T)));
  if (tempArray == nullptr) {
    throw ErrorMemoryAlloc("Error memory allocation");
  }
  if (isAbleToMemcpy()) {
    memcpy(tempArray, &value, sizeof(T));
    if (arraySize != 0) {
      memcpy(tempArray + 1, pArray, arraySize * sizeof(T));
    }
  } else {
    static_assert(std::is_copy_constructible<T>::value,
                  "type T has no copy constructor");
    new (tempArray) T{value};
    for (size_t index = 0; index < arraySize; ++index) {
      new (tempArray + index + 1) T{*(pArray + index)};
    }
  }
  size_t tempArraySize{arraySize};
  destroyArray();
  pArray = tempArray;
  arraySize = ++tempArraySize;
  arrayCapacity = cap;
}
//----------------------------------------------------------------------
template <typename T>
template <typename... T1>
void vector<T>::emplace_back(T1 &&... data) {
  if (arraySize == arrayCapacity) {
    reserve(arrayCapacity * MAKE_DOUBLE);
  }
  new (pArray + arraySize) T{std::forward<T1>(data)...};
  ++arraySize;
}
//----------------------------------------------------------------------
template <typename T> T *vector<T>::insert(T *pos, T value) {
  if (pos == nullptr) {
    throw std::out_of_range("incoming pointer is not valid");
  }
  if (pos == (pArray + arraySize)) {
    push_back(value);
    return (pArray + arraySize);
  }
  if (pos == pArray) {
    push_front(value);
    return pArray;
  }
  size_t insertBeforeIndex{};
  for (size_t index = 1; index < arraySize; ++index) {
    if (pos == (pArray + index)) {
      insertBeforeIndex = index;
      break;
    }
  }
  if (insertBeforeIndex == 0) {
    throw std::out_of_range{"no match with incoming pointer"};
  }
  static_assert(std::is_copy_assignable<T>::value,
                "type T is not copy assignable");
  resize(arraySize + 1);
  for (size_t rIndex = (arraySize - 1); rIndex >= insertBeforeIndex; --rIndex) {
    *(pArray + rIndex) = *(pArray + rIndex - 1);
  }
  *(pArray + insertBeforeIndex) = value;
  return (pArray + insertBeforeIndex);
}
//----------------------------------------------------------------------
template <typename T> T *vector<T>::erase(size_t pos) {
  if (pos >= arraySize) {
    throw std::out_of_range{"incoming position is out of range of vector"};
  }
  static_assert(std::is_copy_assignable<T>::value,
                "type T is not copy assignable");
  (pArray + pos)->~T();
  for (size_t index = pos; index < arraySize - 1; ++index) {
    *(pArray + index) = *(pArray + index + 1);
  }
  --arraySize;
  return (pArray + pos);
}
//----------------------------------------------------------------------
template <typename T> T *vector<T>::erase(T *pos) {
  if (pos == nullptr) {
    throw std::out_of_range{"incoming pointer is not valid"};
  }
  if (pos == (pArray + arraySize)) {
    throw std::out_of_range{"incoming pointer is not valid"};
  }
  bool isErased{};
  for (size_t index = 0; index < arraySize; ++index) {
    if (pos == (pArray + index)) {
      pos->~T();
      isErased = true;
    }
    if (isErased) {
      if (index == arraySize - 1) {
        break;
      }
      *(pArray + index) = *(pArray + index + 1);
    }
  }
  if (!isErased) {
    throw std::out_of_range{"no match with incoming pointer"};
  };
  --arraySize;
  return pos;
}
//----------------------------------------------------------------------
template <typename T> T *vector<T>::erase(T *begin, T *end) {
  if (begin == nullptr || end == nullptr || (begin == end)) {
    throw std::out_of_range{"incoming pointer is not valid"};
  }
  size_t beginIndex{};
  size_t endIndex{};
  bool isBeginIndex{};
  for (size_t index = 0; index <= arraySize; ++index) {
    if ((beginIndex == 0) && ((pArray + index) == begin)) {
      beginIndex = index;
      isBeginIndex = true;
    }
    if ((endIndex == 0) && isBeginIndex && ((pArray + index) == end)) {
      endIndex = index;
    }
  }
  if (endIndex == 0) {
    throw std::out_of_range{"no match with incoming pointer"};
  }
  for (size_t index = beginIndex; index < arraySize; ++index) {
    if (index < endIndex) {
      (pArray + index)->~T();
      continue;
    }
    static_assert(std::is_copy_assignable<T>::value,
                  "type T is not copy assignable");
    *(pArray + index - (endIndex - beginIndex)) = *(pArray + index);
  }
  arraySize -= endIndex - beginIndex;
  return begin;
}
//----------------------------------------------------------------------
template <typename T> void vector<T>::resize(size_t count) {
  if (count == arraySize) {
    return;
  }
  if (count < arraySize) {
    for (size_t index = count - 1; index < arraySize; ++index) {
      (pArray + index)->~T();
    }
    arraySize = count;
    return;
  }
  static_assert(std::is_default_constructible<T>::value,
                "type T has no default constructor");
  if (count <= arrayCapacity) {
    for (size_t index = arraySize; index < count; ++index) {
      new (pArray + index) T{};
    }
    arraySize = count;
    return;
  }
  if (count > arrayCapacity) {
    reserve(count);
    for (size_t index = arraySize; index < count; ++index) {
      new (pArray + index) T{};
    }
  }
}
//----------------------------------------------------------------------
template <typename T> void vector<T>::reserve(size_t new_cap) {
  if (new_cap <= arrayCapacity) {
    return;
  }
  T *tempArray = static_cast<T *>(malloc(new_cap * sizeof(T)));
  if (tempArray == nullptr) {
    throw ErrorMemoryAlloc("Error memory allocation");
  }
  if (isAbleToMemcpy()) {
    memcpy(tempArray, pArray, arraySize * sizeof(T));
  } else {
    static_assert(std::is_copy_constructible<T>::value,
                  "type T has no copy constructor");
    for (size_t index = 0; index < arraySize; ++index) {
      new (tempArray + index) T{*(pArray + index)};
    }
  }
  size_t tempArrSize = arraySize;
  destroyArray();
  pArray = tempArray;
  arraySize = tempArrSize;
  arrayCapacity = new_cap;
}
//----------------------------------------------------------------------
template <typename T> bool vector<T>::isAbleToMemcpy() {
  return (std::is_trivial<T>::value || std::is_standard_layout<T>::value ||
          std::is_pod<T>::value);
}
//----------------------------------------------------------------------
template <typename T> void vector<T>::destroyArray() {
  for (size_t index = 0; index < arraySize; ++index) {
    (pArray + index)->~T();
  }
  free(pArray);
  pArray = nullptr;
  arraySize = 0;
  arrayCapacity = 0;
}
//----------------------------------------------------------------------
