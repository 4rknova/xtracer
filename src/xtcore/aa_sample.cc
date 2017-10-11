#include "aa_sample.h"

namespace xtcore {
    namespace antialiasing {

void sample_set_t::push(sample_t &s)
{
    m_samples.push(s);
}

void sample_set_t::pop(sample_t &s)
{
    s = m_samples.front();
    m_samples.pop();
}

size_t sample_set_t::count()
{
    return m_samples.size();
}

void sample_set_t::clear()
{
    while(m_samples.size() > 0) m_samples.pop();
}

    } /* namespace antialiasing */
} /* namespace xtcore */
