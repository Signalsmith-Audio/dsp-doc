// Copyright Vadim Zavalishin, year unknown
// Posted on KVR: https://www.kvraudio.com/forum/viewtopic.php?p=7523506#p7523506

#include<array>
#include<assert.h>

/*

writepos  scan    scanpos  scanend   outpos  outrange   scanrange   inrange
   0       --        8        5        1       1..4       4..8 <       0
   1       8 7       7        5        2       2..4       5..8        0..1
   2       7 6       6        5        3       3..4       5..8        0..2
   3       6 5       5        5        4        4         5..8        0..3
   4       --        4        0        5       5..8       0..3 <       4
   5       4 3       3        0        6       6..8       0..3        4..5
   6       3 2       2        0        7       7..8       0..3        4..6
   7       2 1       1        0        8        8         0..3        4..7
   8       1 0       0        0        0       0..4       0..3        4..8

writepos  scan    scanpos  scanend   outpos  outrange   scanrange   inrange
   0       --        7        4        1       1..3       4..7 <       0
   1       7 6       6        4        2       2..3       4..7        0..1
   2       6 5       5        4        3        3         4..7        0..2
   3       5 4       4        4        4       4..7       4..7        0..3
   4       --        3        0        5       5..7       0..3 <       4
   5       3 2       2        0        6       6..7       0..3        4..5
   6       2 1       1        0        7        7         0..3        4..6
   7       1 0       0        0        0       0..3       0..3        4..7

*/

template<class T>
class MovingMax
{
public:
    enum { N = 8 };
private:
    enum {
        SCANSTARTLOW = (N-1)/2,
        SCANSTARTHIGH = N-1,
        SCANSTARTXOR = SCANSTARTLOW ^ SCANSTARTHIGH
    };
public:
    MovingMax()
    {
        m_scanmax = m_inmax = std::numeric_limits<T>::lowest();
        m_data.fill(m_inmax);
        m_writepos = 0;
        m_scanend = 0;
        m_scanpos = 0;
        m_scanstart = SCANSTARTLOW;
    }
    T operator<<(T x)
    {
        if( --m_scanpos >= m_scanend )
        {
            m_inmax = std::max(m_inmax,x);
            m_data[m_scanpos] = std::max( m_data[m_scanpos], m_data[m_scanpos+1] );
        }
        else
        {
            m_scanmax = m_inmax;
            m_inmax = x;
            m_scanend = m_scanend ^ ((N+1)/2);
            m_scanstart = m_scanstart ^ SCANSTARTXOR;
            m_scanpos = m_scanstart;
        }
 
        m_data[m_writepos] = x;
        if( ++m_writepos >= N )
            m_writepos = 0;
        T outmax = m_data[m_writepos];
        T movingmax = std::max( m_inmax, std::max(m_scanmax,outmax) );
        return movingmax;
    }
private:
    int m_writepos;
    int m_scanpos;
    int m_scanend;
    int m_scanstart;
    T m_inmax;
    T m_scanmax;
    std::array<T,N> m_data;
};

/*----------------------------------------*/

/// Signalsmith: adapted the above code to give it dynamic size for benchmarking

template<class T>
class MovingMaxDynamic {
private:
	int N;
    int SCANSTARTLOW, SCANSTARTHIGH, SCANSTARTXOR;
public:
    MovingMaxDynamic(int N) : N(N), SCANSTARTLOW((N-1)/2), SCANSTARTHIGH(N-1), SCANSTARTXOR(SCANSTARTLOW ^ SCANSTARTHIGH), m_data(N)
    {
        m_scanmax = m_inmax = std::numeric_limits<T>::lowest();
        m_data.assign(N, m_inmax);
        m_writepos = 0;
        m_scanend = 0;
        m_scanpos = 0;
        m_scanstart = SCANSTARTLOW;
    }
    T operator<<(T x)
    {
        if( --m_scanpos >= m_scanend )
        {
            m_inmax = std::max(m_inmax,x);
            m_data[m_scanpos] = std::max( m_data[m_scanpos], m_data[m_scanpos+1] );
        }
        else
        {
            m_scanmax = m_inmax;
            m_inmax = x;
            m_scanend = m_scanend ^ ((N+1)/2);
            m_scanstart = m_scanstart ^ SCANSTARTXOR;
            m_scanpos = m_scanstart;
        }
 
        m_data[m_writepos] = x;
        if( ++m_writepos >= N )
            m_writepos = 0;
        T outmax = m_data[m_writepos];
        T movingmax = std::max( m_inmax, std::max(m_scanmax,outmax) );
        return movingmax;
    }
private:
    int m_writepos;
    int m_scanpos;
    int m_scanend;
    int m_scanstart;
    T m_inmax;
    T m_scanmax;
    std::vector<T> m_data;
};
