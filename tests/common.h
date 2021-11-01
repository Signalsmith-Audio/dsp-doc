#ifndef SKIP_CSV
#include <fstream>
class CsvWriter {
	std::ofstream csvFile;
	bool newLine = true;
public:
	CsvWriter(std::string name) : csvFile(name + ".csv") {}
	
	CsvWriter & write() {
		return *this;
	}
	template<class First, class... Args>
	CsvWriter & write(const First &v, Args ...args) {
		if (!newLine) csvFile << ",";
		newLine = false;
		csvFile << v;
		return write(args...);
	}
	template<typename... T>
	CsvWriter & line(T... t) {
		write(t...);
		csvFile << "\n";
		newLine = true;
		return *this;
	}
};
#else
// Don't do anything
struct CsvWriter {
	CsvWriter(std::string name) {}
	template<typename... T>
	CsvWriter & write(T ...t) {
		return *this;
	}
	template<typename... T>
	CsvWriter & line(T ...t) {
		return *this;
	}
};
#endif
