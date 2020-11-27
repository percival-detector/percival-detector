
#pragma once

#define FRAME_ROWS 1484
#define FRAME_COLS 1408
#define FRAMER_COLS 1440

#define BOUNDS_CHECK 1

#include <log4cxx/logger.h>

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

    void init(log4cxx::LoggerPtr logger, int rows, int cols, void* ptr=nullptr)
    {
        m_logger = logger;
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

    // this can load a 2 or 3d dataset. frameNo is only used with 3d datasets.
    int64_t loadFromH5(std::string filename, std::string dataset, int frameNo);

    void setAll(T val)
    {
        if(m_data)
        {
            for(int r=0;r<m_rows;++r)
                for(int c=0;c<m_cols;++c)
                    at(r,c) = val;
        }
    }

    FrameMem(const FrameMem&) = delete;

    FrameMem& operator=(FrameMem&& rhs)
    {
        // this is copy-assignment move operator
        memcpy(this, &rhs, sizeof(FrameMem));
        rhs.m_ownMemory = false;
        rhs.m_rows = rhs.m_cols = 0;
        rhs.m_data = nullptr;
    }


    void clone(FrameMem& source)
    {
        init(source.m_logger, source.rows(), source.cols());
        memcpy(data(), source.data(), m_dataQty * sizeof(T));
    }

    T& at(int r, int c)
    {
#if BOUNDS_CHECK
        if(r < 0 || rows() <= r)
        {
            LOG4CXX_ERROR(m_logger, "out of bounds: row " << r << " in FrameMem with rows " << rows());
        }
        if(c < 0 || cols() <= c)
        {
            LOG4CXX_ERROR(m_logger, "out of bounds: col " << c << " in FrameMem with cols " << cols());
        }
#endif
        return at(r * m_cols + c);
    }
    T& at(int idx)
    {
#if BOUNDS_CHECK
        if(idx < 0 || m_dataQty <= idx)
        {
            LOG4CXX_ERROR(m_logger, "out of bounds at " << idx << " in FrameMem with capacity " << m_dataQty);
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

    log4cxx::LoggerPtr m_logger;
};

typedef FrameMem<float> MemBlockF;
typedef FrameMem<double> MemBlockD;
// rename MemBlockI16
typedef FrameMem<uint16_t> MemBlockI16;

