#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, std::string data, bool is_last_substring) {
    Writer &w = output_.writer();
    if (is_last_substring) {
        last_index = first_index + data.size();
    }
    // Special case: empty final segment
    if (is_last_substring && data.empty()&& next_index >= last_index) {
        w.close();
        return;
    }

    // 1. Window (capacity)
    uint64_t window_start = next_index;
    uint64_t window_end   = window_start + w.available_capacity();

    // 2. Trim to fit capacity window
    uint64_t start = std::max(first_index, window_start);
    uint64_t end   = std::min(first_index + data.size(), window_end);
    if (start >= end) return;  // nothing useful
    if (start < first_index) start = first_index;
    std::string trimmed = data.substr(start - first_index, end - start);

    // ------------------------------
    // Case A: arrives after next_index (gap -> buffer only)
    // ------------------------------
    if (start > next_index) {
        // merge with existing buffered intervals
        auto it = buffer.lower_bound(start);
        if (it != buffer.begin()) --it;
        while (it != buffer.end() && it->first <= end) {
            uint64_t seg_start = it->first;
            uint64_t seg_end   = seg_start + it->second.size();
            if (seg_end < start) { ++it; continue; }

            // merge overlap
            start   = std::min(start, seg_start);
            end     = std::max(end, seg_end);
            trimmed = data.substr(start - first_index, end - start);
            it = buffer.erase(it);
        }
        buffer[start] = trimmed;
    }

    // ------------------------------
    // Case B: overlaps with next_index (can flush immediately)
    // ------------------------------
    else { // start <= next_index
        // push as much contiguous data as possible
        buffer[start] = trimmed;

        while (!buffer.empty() && buffer.begin()->first <= next_index) {
            auto it = buffer.begin();
            uint64_t seg_start = it->first;
            string &seg   = it->second;  
            if (seg_start > next_index) break;

            // if seg overlaps next_index, cut off the already written part
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

    // 3. Handle end of stream

    if (next_index >= last_index && last_index != 0) {
        w.close();   // tells reader: finished
    }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total = 0;
    for (const auto &kv : buffer) {
        total += kv.second.size();
    }
    return total;
}
