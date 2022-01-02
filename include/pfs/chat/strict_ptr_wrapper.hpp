////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace chat {

template <typename T>
class basic_strict_ptr_wrapper
{
protected:
    T * _p {nullptr};

public:
    basic_strict_ptr_wrapper (T & r) : _p(& r) {}
    ~basic_strict_ptr_wrapper () { _p = nullptr; }

    basic_strict_ptr_wrapper () = delete;
    basic_strict_ptr_wrapper (basic_strict_ptr_wrapper const & ) = delete;
    basic_strict_ptr_wrapper & operator = (basic_strict_ptr_wrapper const & ) = delete;
    basic_strict_ptr_wrapper (basic_strict_ptr_wrapper && ) = default;
    basic_strict_ptr_wrapper & operator = (basic_strict_ptr_wrapper && ) = delete;

    operator bool ()
    {
        return _p != nullptr;
    }
};

template <typename T>
class strict_ptr_wrapper: public basic_strict_ptr_wrapper<T>
{
public:
    using basic_strict_ptr_wrapper<T>::basic_strict_ptr_wrapper;

    T & operator * () { return *this->_p; }
    T * operator -> () { return this->_p; }
};

template <typename T>
class strict_ptr_wrapper<T const>: public basic_strict_ptr_wrapper<const T>
{
public:
    using basic_strict_ptr_wrapper<T const>::basic_strict_ptr_wrapper;

    T & operator * () const { return *this->_p; }
    T * operator -> () const { return this->_p; }
};

} // namespace chat
