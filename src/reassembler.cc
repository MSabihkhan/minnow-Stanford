#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, std::string data, bool is_last_substring) {
    Writer &w = output_.writer();
    if (is_last_substring) {
        last_index = first_index + data.size();
    }
    if (is_last_substring && data.empty()&& next_index >= last_index) {
        w.close();
        return;
    }

    uint64_t window_start = next_index;
    uint64_t window_end   = window_start + w.available_capacity();
    if (first_index >= window_end || first_index + data.size() <= window_start) {
        return;
    }

    uint64_t start = std::max(first_index, window_start);
    uint64_t end   = std::min(first_index + data.size(), window_end);
    if (start >= end) return; 
    if (start < first_index) start = first_index;
    string trimmed = data.substr(start - first_index, end - start);

    if (start > next_index) {
        auto it = buffer.lower_bound(start);
        if (it != buffer.begin()) --it;
        while (it != buffer.end() && it->first <= end) {
            uint64_t seg_start = it->first;
            uint64_t seg_end   = seg_start + it->second.size();
            if (seg_end < start) { ++it; continue; }

            start   = std::min(start, seg_start);
            end     = std::max(end, seg_end);
            if (start < first_index) start = first_index;
            trimmed = data.substr(start - first_index, end - start);
            it = buffer.erase(it);
        }
        buffer[start] = trimmed;
    }
    else { 
        buffer[start] = trimmed;

        while (!buffer.empty() && buffer.begin()->first <= next_index) {
            auto it = buffer.begin();
            uint64_t seg_start = it->first;
            string &seg   = it->second;  
            if (seg_start > next_index) break;

            if (seg_start < next_index) {
                size_t skip = next_index - seg_start;
                if (skip >= seg.size()) {
                    buffer.erase(buffer.begin());
                    continue;
                }
                seg = seg.substr(skip);
            }

            w.push(seg);
            next_index += seg.size();
            buffer.erase(buffer.begin());
        }
    }

    if (next_index >= last_index && last_index != 0) {
        w.close(); 
    }
}

uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total = 0;
    for (const auto &kv : buffer) {
        total += kv.second.size();
    }
    return total;
}
