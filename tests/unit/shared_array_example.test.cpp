#include <catch.hpp>
#include <flexclass.hpp>

#include <atomic>

template<class T> class SharedArray;
template<class T>
class SharedArray<T[]>
{
    struct Impl
    {
        auto fc_handles() { return fc::make_tuple(&data); }
        unsigned refCount;
        std::conditional_t<std::is_trivially_destructible_v<T>,
                          fc::Array<T>,
                          fc::Range<T>> data;
    };

  public:
    /* Interesting public API */
    static SharedArray make(std::size_t len) { return {fc::make<Impl>(len)(/*num references*/1u)}; }

    decltype(auto) operator[](std::size_t i) { return m_data->data.begin()[i]; }
    decltype(auto) operator[](std::size_t i) const { return m_data->data.begin()[i]; }

    auto use_count() const { return m_data ? m_data->refCount : 0; }

    /* Boilerplate */
    SharedArray(SharedArray&& other) : m_data(std::exchange(other.m_data, nullptr)) {}
    SharedArray(const SharedArray& other) { m_data = other.m_data; incr(); }
    SharedArray& operator=(SharedArray&& other) { decr(); m_data = std::exchange(other.m_data, nullptr); return *this; }
    SharedArray& operator=(const SharedArray& other) { decr(); m_data = other.m_data; incr(); return *this; }
    ~SharedArray() { decr(); }
  private:
    SharedArray(Impl* data) : m_data(data) {}
    void incr() { if (m_data) m_data->refCount++; }
    void decr() { if (m_data && m_data->refCount-- == 1) fc::destroy(m_data); }
    Impl* m_data {nullptr};
};

TEST_CASE( "Exercise the SharedArray", "[shared_array_example]" )
{
    auto sa1 = SharedArray<char[]>::make(100);
    CHECK(sa1.use_count() == 1);

    auto sa2 = std::move(sa1);
    CHECK(sa1.use_count() == 0);
    CHECK(sa2.use_count() == 1);

    {
        auto sa3 = sa2;
        CHECK(sa2.use_count() == 2);
        CHECK(sa3.use_count() == 2);

        for (int i = 0; i < 100; ++i) sa2[i] = i;
        for (int i = 0; i < 100; ++i) CHECK(sa3[i] == i);
    }

    CHECK(sa2.use_count() == 1);
}

template<class T> class SharedRange;
template<class T>
class SharedRange<T[]>
{
    struct Impl
    {
        auto fc_handles() { return fc::make_tuple(&data); }
        std::atomic<unsigned> refCount;
        fc::AdjacentRange<T> data;
    };

  public:
    /* Interesting public API */
    static SharedRange make(std::size_t len) { return {fc::make<Impl>(len)(/*num references*/1u)}; }

    auto begin() { return m_data->data.begin(m_data); }
    auto end() { return m_data->data.end(m_data); }

    auto use_count() const { return m_data ? m_data->refCount.load() : 0; }

    /* Boilerplate */
    SharedRange(SharedRange&& other) : m_data(std::exchange(other.m_data, nullptr)) {}
    SharedRange(const SharedRange& other) { m_data = other.m_data; incr(); }
    SharedRange& operator=(SharedRange&& other) { decr(); m_data = std::exchange(other.m_data, nullptr); return *this; }
    SharedRange& operator=(const SharedRange& other) { decr(); m_data = other.m_data; incr(); return *this; }
    ~SharedRange() { decr(); }
  private:
    SharedRange(Impl* data) : m_data(data) {}
    void incr() { if (m_data) m_data->refCount++; }
    void decr() { if (m_data && m_data->refCount-- == 1) fc::destroy(m_data); }
    Impl* m_data {nullptr};
};

TEST_CASE( "Exercise the SharedRange", "[shared_range_example]" )
{
    auto sa1 = SharedRange<char[]>::make(100);
    CHECK(sa1.use_count() == 1);

    auto sa2 = std::move(sa1);
    CHECK(sa1.use_count() == 0);
    CHECK(sa2.use_count() == 1);

    {
        auto sa3 = sa2;
        CHECK(sa2.use_count() == 2);
        CHECK(sa3.use_count() == 2);

        int i = 0;
        std::for_each(sa2.begin(), sa2.end(), [&] (auto& c) { c = i++; });

        i = 0;
        std::for_each(sa2.begin(), sa2.end(), [&] (auto& c) { CHECK(c == i++); });
    }

    CHECK(sa2.use_count() == 1);
}
