#include "reassembler.hh"
#include "debug.hh"
#include<algorithm>
#include<vector>
using namespace std;

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {
    Writer &w = output_.writer();
    if (is_last_substring) {
        last_index = first_index + data.size();
    }
    if (is_last_substring && data.empty() && next_index >= last_index) {
        w.close();
        return;
    }

    uint64_t window_start = next_index;
    uint64_t window_end = window_start + w.available_capacity();
    if (first_index >= window_end || first_index + data.size() <= window_start) {
        return;
    }
    
    uint64_t start = max(first_index, window_start);
    uint64_t end = min(first_index + data.size(), window_end);
    if (start >= end) {
        return;
    }
    
    string trimmed = data.substr(start - first_index, end - start);
    
    if (start > next_index) {
        uint64_t new_si = start;
        uint64_t new_ei = end;
        vector<map<uint64_t, string>::iterator> to_remove;
        
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            uint64_t seg_start = it->first;
            uint64_t seg_end = seg_start + it->second.size();
            if (new_si < seg_end && new_ei > seg_start) {
                to_remove.push_back(it);
                new_si = min(new_si, seg_start);
                new_ei = max(new_ei, seg_end);
            }
        }
        string merged(new_ei - new_si, '\0');
        for (size_t i = 0; i < trimmed.size(); ++i) {
            merged[start - new_si + i] = trimmed[i];
        }
        for (auto it : to_remove) {
            uint64_t seg_start = it->first;
            const string& seg_data = it->second;
            for (size_t i = 0; i < seg_data.size(); ++i) {
                merged[seg_start - new_si + i] = seg_data[i];
            }
        }
        for (auto it : to_remove) {
            buffer.erase(it);
        }
        buffer[new_si] = merged;
    }
    else {
        buffer[start] = trimmed;
        while (!buffer.empty() && w.available_capacity() > 0) {
            auto it = buffer.begin();
            uint64_t seg_start = it->first;
            string &seg = it->second;
            if (seg_start > next_index){
                break;
            }
            if (seg_start < next_index) {
                size_t skip = next_index - seg_start;
                if (skip >= seg.size()) {
                    buffer.erase(it);
                    continue;
                }
                seg = seg.substr(skip);
                seg_start = next_index;
            }
            size_t size = min(seg.size(), w.available_capacity());
            if (size == 0) break;

            w.push(seg.substr(0, size));
            next_index += size;

            if (size < seg.size()) {
                buffer[next_index] = seg.substr(size);
                buffer.erase(it);
                break;
            } else {
                buffer.erase(it);
            }
        }
    }
    if (next_index >= last_index && last_index != 0) {
        w.close();
    }
}

uint64_t Reassembler::count_bytes_pending() const {
    uint64_t total = 0;
    for (const auto &kv : buffer) {
        total += kv.second.size();
    }
    return total;
}