#ifndef FB_EXPLAINABLES_H
#define FB_EXPLAINABLES_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
using namespace std;

namespace fb {
    // Forward declarations
    class Details;
    class StringDetails;
    class DictDetails;
    class Explainable;

    // Details class
    class Details {
        virtual Details* copy_impl() const = 0;
    public:
        virtual ~Details() {}
        unique_ptr<Details> copy() const;
        virtual string str() const = 0;
    };
    
    // Explainable class
    class Explainable {
        double value;
        unique_ptr<Details> details;

    public:
        Explainable();
        Explainable(int value);
        Explainable(double value);
        Explainable(double value, const string& description);
        Explainable(double value, const unordered_map<string, Explainable>& description);
        Explainable(const Explainable& prototype);
        Explainable(Explainable&& prototype) noexcept;

        Explainable& operator=(const Explainable& prototype) noexcept;
        Explainable& operator=(Explainable&& prototype) noexcept;

        double get() const;
        virtual string str() const;
    };

    // StringDetails class
    class StringDetails : public Details {
        string description;
        Details* copy_impl() const override;
    public:
        StringDetails(const string& description);
        string str() const override;
    };

    // DictDetails class
    class DictDetails : public Details {
        unordered_map<string, Explainable> values;
        Details* copy_impl() const override;
    public:
        DictDetails(const unordered_map<string, Explainable>& values);
        string str() const override;
    };

    // Implementations
    unique_ptr<Details> Details::copy() const { return unique_ptr<Details>(this->copy_impl()); }

    Details* StringDetails::copy_impl() const { return new StringDetails(description); }
    StringDetails::StringDetails(const string& description) : description(description) {}
    string StringDetails::str() const { return description; }

    Details* DictDetails::copy_impl() const { return new DictDetails(values); }
    DictDetails::DictDetails(const unordered_map<string, Explainable>& values) : values(values) {}
    string DictDetails::str() const {
        string desc = "";
        for (const auto &entry : values) {
            std::ostringstream oss;
            oss << "\n  |" << std::left << std::setw(19) << entry.first
                << to_string(entry.second.get()) + " (" + entry.second.str() + ")";
            desc += oss.str();
        }
        return desc;
    }

    Explainable::Explainable() : value(0), details(nullptr) {}
    Explainable::Explainable(int value) : value(value), details(nullptr) {}
    Explainable::Explainable(double value) : value(value), details(nullptr) {}
    Explainable::Explainable(double value, const string& description)
        : value(value), details(make_unique<StringDetails>(description)) {}
    Explainable::Explainable(double value, const unordered_map<string, Explainable>& description)
        : value(value), details(make_unique<DictDetails>(description)) {}
    Explainable::Explainable(const Explainable& prototype)
        : value(prototype.value), details(prototype.details ? prototype.details->copy() : nullptr) {}
    Explainable::Explainable(Explainable&& prototype) noexcept
        : value(prototype.value), details(std::move(prototype.details)) {}

    Explainable& Explainable::operator=(const Explainable& prototype) noexcept {
        if (this != &prototype) {
            value = prototype.value;
            details = prototype.details ? prototype.details->copy() : nullptr;
        }
        return *this;
    }

    Explainable& Explainable::operator=(Explainable&& prototype) noexcept {
        if (this != &prototype) {
            value = prototype.value;
            details = std::move(prototype.details);
        }
        return *this;
    }

    double Explainable::get() const { return value; }
    string Explainable::str() const { return details ? details->str() : ""; }

} // fb

#endif // FB_EXPLAINABLES_H
