#ifndef VERSION_H_
#define VERSION_H_

#include "pal.h"
#include "utils.h"

namespace coreload
{
    struct version_t
    {
        version_t();
        version_t(int major, int minor, int build, int revision);

        int get_major() const { return m_major; }
        int get_minor() const { return m_minor; }
        int get_build() const { return m_build; }
        int get_revision() const { return m_revision; }

        void set_major(int m) { m_major = m; }
        void set_minor(int m) { m_minor = m; }
        void set_build(int m) { m_build = m; }
        void set_revision(int m) { m_revision = m; }

        pal::string_t as_str() const;

        bool operator ==(const version_t& b) const;
        bool operator !=(const version_t& b) const;
        bool operator <(const version_t& b) const;
        bool operator >(const version_t& b) const;
        bool operator <=(const version_t& b) const;
        bool operator >=(const version_t& b) const;

        static bool parse(const pal::string_t& ver, version_t* ver_out);

    private:
        int m_major;
        int m_minor;
        int m_build;
        int m_revision;

        static int compare(const version_t&a, const version_t& b);
    };

} // namespace coreload

#endif // VERSION_H_