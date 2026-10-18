#ifndef BOOST_PROGRAM_OPTIONS_HPP
#define BOOST_PROGRAM_OPTIONS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <memory>

namespace boost {
namespace program_options {

    class error : public std::runtime_error {
    public:
        explicit error(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    struct value_semantic {
        virtual ~value_semantic() = default;
    };

    template <typename T>
    class typed_value : public value_semantic {
    public:
        typed_value() = default;
    };

    template <typename T>
    inline std::shared_ptr<value_semantic> value() {
        return std::make_shared<typed_value<T>>();
    }

    class options_description_easy_init {
    public:
        std::unordered_map<std::string, std::shared_ptr<value_semantic>>& options;

        explicit options_description_easy_init(std::unordered_map<std::string, std::shared_ptr<value_semantic>>& opts)
            : options(opts) {}

        options_description_easy_init& operator()(const char* name,
                                                  std::shared_ptr<value_semantic> sem = nullptr,
                                                  const char* /*description*/ = "") {
            options[name] = sem;
            return *this;
        }
    };

    class options_description {
    public:
        std::string caption;
        std::unordered_map<std::string, std::shared_ptr<value_semantic>> options;

        explicit options_description(const std::string& cap = "") : caption(cap) {}

        options_description_easy_init add_options() {
            return options_description_easy_init(options);
        }
    };

    class variable_value {
    private:
        std::string str_val;
    public:
        variable_value() = default;
        explicit variable_value(std::string val) : str_val(std::move(val)) {}

        template <typename T>
        T as() const {
            if constexpr (std::is_same_v<T, std::string>) {
                return str_val;
            } else if constexpr (std::is_same_v<T, int>) {
                return std::stoi(str_val);
            } else if constexpr (std::is_same_v<T, unsigned int>) {
                return static_cast<unsigned int>(std::stoul(str_val));
            } else if constexpr (std::is_same_v<T, size_t>) {
                return static_cast<size_t>(std::stoull(str_val));
            } else if constexpr (std::is_same_v<T, double>) {
                return std::stod(str_val);
            } else if constexpr (std::is_same_v<T, float>) {
                return std::stof(str_val);
            } else {
                std::istringstream iss(str_val);
                T res;
                iss >> res;
                return res;
            }
        }
    };

    class variables_map {
    public:
        std::unordered_map<std::string, variable_value> values;

        size_t count(const std::string& name) const {
            return values.count(name);
        }

        const variable_value& operator[](const std::string& name) const {
            auto it = values.find(name);
            if (it == values.end()) {
                throw error("Option not found: " + name);
            }
            return it->second;
        }

        variable_value& operator[](const std::string& name) {
            return values[name];
        }
    };

    struct parsed_options {
        options_description description;
        std::vector<std::pair<std::string, std::string>> options;
    };

    inline parsed_options parse_command_line(int argc, char* argv[], const options_description& desc) {
        parsed_options po;
        po.description = desc;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--", 0) == 0) {
                std::string key_val = arg.substr(2);
                auto eq_pos = key_val.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = key_val.substr(0, eq_pos);
                    std::string val = key_val.substr(eq_pos + 1);
                    po.options.emplace_back(key, val);
                } else {
                    std::string key = key_val;
                    if (i + 1 < argc && argv[i + 1][0] != '-') {
                        po.options.emplace_back(key, argv[++i]);
                    } else {
                        po.options.emplace_back(key, "1");
                    }
                }
            } else if (arg.rfind("-", 0) == 0 && arg.length() > 1) {
                std::string key = arg.substr(1);
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    po.options.emplace_back(key, argv[++i]);
                } else {
                    po.options.emplace_back(key, "1");
                }
            }
        }
        return po;
    }

    inline void store(const parsed_options& po, variables_map& vm) {
        for (const auto& kv : po.options) {
            vm[kv.first] = variable_value(kv.second);
        }
    }

    inline void notify(variables_map& /*vm*/) {
    }

} // namespace program_options
} // namespace boost

#endif // BOOST_PROGRAM_OPTIONS_HPP
