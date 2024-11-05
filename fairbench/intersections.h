/**
    Copyright 2024 Emmanouil Krasanakis

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef FB_INTERSECTIONS
#define FB_INTERSECTIONS


namespace fb {
    using SensitiveMap = std::unordered_map<std::string, Vec>;

    // Function to generate all non-empty subsets of keys with size >= 2
    std::vector<std::vector<std::string>> generate_subsets(const std::vector<std::string>& keys) {
        std::vector<std::vector<std::string>> subsets;
        int n = keys.size();
        // Start from 1 to (2^n -1), but we want subsets with size >=2
        for(int mask = 1; mask < (1 << n); ++mask){
            std::vector<std::string> subset;
            for(int i = 0; i < n; ++i){
                if(mask & (1 << i)){
                    subset.push_back(keys[i]);
                }
            }
            if(subset.size() >=2){
                subsets.push_back(subset);
            }
        }
        return subsets;
    }

    // Function to compute element-wise product of multiple vectors
    Vec compute_intersection(const std::vector<Vec>& vectors){
        if(vectors.empty()) return {};
        size_t size = vectors[0].size();
        Vec result(size, 1.0); // Initialize with 1 for multiplication
        for(const auto& vec : vectors){
            for(size_t i = 0; i < size; ++i){
                result[i] *= vec[i];
            }
        }
        return result;
    }

    // Function to create all non-empty intersections and add them to Sensitive map
    void add_intersections(SensitiveMap& Sensitive){
        // Step 1: Collect all keys
        std::vector<std::string> keys;
        for(const auto& pair : Sensitive){
            keys.push_back(pair.first);
        }

        // Step 2: Generate all subsets with size >=2
        std::vector<std::vector<std::string>> subsets = generate_subsets(keys);

        // Step 3: For each subset, compute the intersection and add to map
        for(const auto& subset : subsets){
            // Create a new key by joining subset keys with "_"
            std::string new_key;
            for(size_t i = 0; i < subset.size(); ++i){
                new_key += subset[i];
                if(i != subset.size() -1){
                    new_key += "_";
                }
            }

            // Check if this intersection already exists to avoid duplicates
            if(Sensitive.find(new_key) != Sensitive.end()){
                continue; // Skip if already exists
            }

            // Collect the vectors corresponding to the subset
            std::vector<Vec> vectors;
            bool valid = true;
            size_t vec_size = 0;
            for(const auto& key : subset){
                auto it = Sensitive.find(key);
                if(it == Sensitive.end()){
                    valid = false;
                    break; // Key not found, skip this subset
                }
                if(vectors.empty()){
                    vec_size = it->second.size();
                } else {
                    if(it->second.size() != vec_size){
                        valid = false;
                        break; // Vector size mismatch
                    }
                }
                vectors.push_back(it->second);
            }

            if(!valid){
                std::cerr << "Invalid subset or vector size mismatch for subset: ";
                for(const auto& key : subset){
                    std::cerr << key << "_";
                }
                std::cerr << "\b \n"; // Remove trailing underscore
                continue;
            }

            // Compute the intersection
            Vec intersection = compute_intersection(vectors);
            double sum = 0;
            for(double value : intersection)
                sum += value;
            if(sum==0)
                continue;

            // Add to Sensitive map
            Sensitive[new_key] = intersection;
        }
    }
} // namespace fb

#endif // FB_INTERSECTIONS