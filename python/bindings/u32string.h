#pragma once

#include <string>

#include <bytesobject.h>
#include <nanobind/nanobind.h>
#include <unicodeobject.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <> struct type_caster<std::u32string>
{
    NB_TYPE_CASTER(std::u32string, const_name("str"))

    bool from_python(handle src, uint8_t, cleanup_list*) noexcept
    {
        Py_ssize_t size;
        PyObject* utf32_str = PyUnicode_AsUTF32String(src.ptr());
        char* str;
        PyBytes_AsStringAndSize(utf32_str, &str, &size);
        if (!str)
        {
            PyErr_Clear();
            return false;
        }
        str += 4; // skip BOM
        size -= 4;
        value = std::u32string(reinterpret_cast<const char32_t*>(str), (size_t)size / sizeof(char32_t));
        return true;
    }

    static handle from_cpp(const std::u32string& value, rv_policy, cleanup_list*) noexcept
    {
        return PyUnicode_DecodeUTF32(reinterpret_cast<const char*>(value.c_str()), (Py_ssize_t)(value.size() * sizeof(char32_t)), nullptr, nullptr);
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
