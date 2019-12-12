
#pragma once

#define FRAME_ROWS 1484
#define FRAME_COLS 1408

#define BOUNDS_CHECK 1

#include <string>
#include <cassert>
#include <iostream>
#include <inttypes.h>
#include <cstring>

// This object holds a block of (row,col) x T elements (row-major). You must call init()
// to activate it: you can give it a pointer to a memory block to use (eg from another
// FrameMem object) or if you give it a nullptr, it will allocate aligned memory itself.
// If it owns its memory, it will free it in the destuctor.

template<typename T>
struct FrameMem
{
    FrameMem()
    {
        m_rows = m_cols = 0;
        m_data = nullptr;
    }

    ~FrameMem()
    {
        if(m_ownMemory)
            free(m_data);
    }

    void init(int rows, int cols, void* ptr=nullptr)
    {
        m_rows = rows; m_cols = cols;
        m_dataQty = m_rows * m_cols;
        if(m_data && m_ownMemory)
        {
            free(m_data);
            m_data = nullptr;
        }
        if(ptr)
        {
            m_data = (T*)ptr;
            m_ownMemory = false;
        }
        else
        {
            int sz = m_dataQty * sizeof(T);
            // put 32-byte alignment here for anyone who wants it eg simd.
            int align = 32;
            // this returns nullptr on failure.
            m_data = (T*)aligned_alloc(align, align*(1+sz/align));
            m_ownMemory = true;
        }
    }

    // for some T, this can load a 2d dataset, and then the frameNo is ignored.
    int64_t loadFromH5(std::string filename, std::string dataset, int frameNo, std::string* outError=nullptr);

    void setAll(T val)
    {
        if(m_data)
        {
            for(int c=0;c<m_cols;++c)
                for(int r=0;r<m_rows;++r)
                    m_data[r*m_cols + c] = val;
        }
    }

    FrameMem(const FrameMem&) = delete;


    void clone(FrameMem& source)
    {
        init(source.rows(), source.cols());
        memcpy(data(), source.data(), m_dataQty * sizeof(T));
    }

    T& at(int r, int c)
    {
        return at(r * m_cols + c);
    }
    T& at(int idx)
    {
#if BOUNDS_CHECK
        if(idx < 0 || m_dataQty <= idx)
        {
            std::cout << "Error out of bounds at " << idx << " in FrameMem with capacity " << m_dataQty << std::endl;
            assert(false);
        }
#endif
        return m_data[idx];
    }
    T* data()
    {
        return m_data;
    }
    int cols() {return m_cols;}
    int rows() {return m_rows;}

private:
    bool m_ownMemory;
    int m_rows, m_cols, m_dataQty;
    T* m_data;
};

typedef FrameMem<float> MemBlockF;
// rename MemBlockI16
typedef FrameMem<uint16_t> MemBlockI16;

