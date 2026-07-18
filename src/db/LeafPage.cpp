#include <db/LeafPage.hpp>
#include <stdexcept>
#include <algorithm>

using namespace db;

namespace {
    // Lightweight strided iterator so std::lower_bound can binary-search over
    // serialized tuples packed into the page, keyed on the int field at key_index.
    struct KeyIterator {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = uint16_t;
        using value_type = int;
        using reference = int &;

        const uint8_t *data;
        size_t width;
        uint16_t slot;

        void operator++() { ++slot; }
        void operator--() { --slot; }
        void operator+=(uint16_t n) { slot += n; }
        void operator-=(uint16_t n) { slot -= n; }
        uint16_t operator-(const KeyIterator &other) const { return slot - other.slot; }
        bool operator<(const KeyIterator &other) const { return slot < other.slot; }
        bool operator==(const KeyIterator &other) const { return slot == other.slot; }

        int operator*() const { return *reinterpret_cast<const int *>(data + slot * width); }
    };
}

LeafPage::LeafPage(Page &page, const TupleDesc &td, size_t key_index) : td(td), key_index(key_index) {
    // TODO pa2
    header = reinterpret_cast<LeafPageHeader *>(page.data());
    capacity = (DEFAULT_PAGE_SIZE - sizeof(LeafPageHeader)) / td.length();
    data = page.data() + DEFAULT_PAGE_SIZE - td.length() * capacity;
}

bool LeafPage::insertTuple(const Tuple &t) {
    // TODO pa2
    int key = std::get<int>(t.get_field(key_index));

    const auto first = data + td.offset_of(key_index);
    const auto width = td.length();
    const auto last = first + header->size * width;
    auto it = std::lower_bound(KeyIterator{first, width, 0}, KeyIterator{last, width, header->size}, key,
                                [](int a, int b) { return a < b; });

    auto slot = it.slot;
    if (slot >= header->size || key != *it) {
        std::copy_backward(data + slot * width, data + header->size * width, data + (header->size + 1) * width);
        ++header->size;
    }
    td.serialize(data + slot * td.length(), t);
    return header->size == capacity;
}

int LeafPage::split(LeafPage &new_page) {
    // TODO pa2
    size_t half = header->size / 2;
    new_page.header->size = header->size - half;
    new_page.header->next_leaf = header->next_leaf;
    std::copy(data + half * td.length(), data + header->size * td.length(), new_page.data);
    header->size = half;
    return std::get<int>(td.deserialize(new_page.data).get_field(key_index));
}

Tuple LeafPage::getTuple(size_t slot) const {
    // TODO pa2
    if (slot >= header->size) {
        throw std::out_of_range("slot out of range");
    }
    return td.deserialize(data + slot * td.length());
}
