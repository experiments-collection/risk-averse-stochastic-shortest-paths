#include "logger.h"

#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <numeric>

struct Names {

	inline static std::string diagram_file_name() {
		return "diagram.tex";
	}

	inline static std::string States{ "States:" };
	inline static std::string Transitions{ "Transitions:" };
	inline static std::string TotalSeconds{ "TotalSeconds" };

};

struct labels {

	inline static std::string TOTAL_TIME_DIV_S{ "total time / s" };
	inline static std::string TOTAL_TIME_DIV_1000S{ "total time / $$10^3$$s" };
	inline static std::string COUNT_STATES_DIV_10000{ "\\#states / 10.000" };
	inline static std::string COUNT_STATES_DIV_10000000{ "\\#states / 10.000.000" };
};

class PrismLogExtractor {
public:

	std::string prism_log;
	std::string time_log;

	uint64_t states;
	uint64_t transitions;
	double total_time_seconds;


	PrismLogExtractor(const std::string& prism_log, const std::string& time_log) : prism_log(prism_log), time_log(time_log) {
		parse();
	}

	PrismLogExtractor() : prism_log(), time_log() {
		throw true;
	}

	void parse() {
		////////////////////// #states
		std::string::size_type first_position_states = prism_log.find(Names::States);
		std::string::size_type first_position_states_end = prism_log.find("(", first_position_states);
		std::string::size_type first_position_states_after_colon = first_position_states + Names::States.size();

		std::string s_states = prism_log.substr(first_position_states_after_colon, first_position_states_end - first_position_states_after_colon);

		std::regex spaces_or_colon("\\s|\\:");

		// write the results to an output iterator
		s_states.erase(std::regex_replace(s_states.begin(), s_states.begin(), s_states.end(), spaces_or_colon, ""), s_states.end());

		states = std::stoull(s_states);

		////////////////////// #transitions
		std::string::size_type first_position_transitions = prism_log.find(Names::Transitions);
		std::string::size_type first_position_transitions_end = prism_log.find("\n", first_position_transitions);
		std::string::size_type first_position_transitions_after_colon = first_position_transitions + Names::Transitions.size();

		std::string s_transitions = prism_log.substr(first_position_transitions_after_colon, first_position_transitions_end - first_position_transitions_after_colon);

		// write the results to an output iterator
		s_transitions.erase(std::regex_replace(s_transitions.begin(), s_transitions.begin(), s_transitions.end(), spaces_or_colon, ""), s_transitions.end());

		transitions = std::stoull(s_transitions);

		////////////////////// #time
		std::string::size_type position_time_s = time_log.find(Names::TotalSeconds);
		std::string::size_type position_time_s_end = time_log.find("\n", position_time_s);
		std::string::size_type position_time_s_behind = position_time_s + Names::TotalSeconds.size();

		std::string s_time = time_log.substr(position_time_s_behind, position_time_s_end - position_time_s_behind);

		s_time.erase(std::regex_replace(s_time.begin(), s_time.begin(), s_time.end(), spaces_or_colon, ""), s_time.end());
		s_time = std::regex_replace(s_time, std::regex(","), ".");

		total_time_seconds = std::stod(s_time);

	}
};

std::string file_to_string(std::filesystem::path path) {
	std::ifstream file;
	file.open(path.string()/*, std::ios::binary*/);
	file >> std::noskipws;
	return std::string(std::istream_iterator<char>(file), std::istream_iterator<char>());
}


struct graph {

	std::vector<std::pair<std::string, std::string>> value_pairs;
	std::string additional_attributes;
	std::string legend_entry;

};

std::string create_diagram(const std::vector<graph>& graphs, const std::string& x_label, const std::string& y_label) {

	const std::string tikz_template{ R"xxx(\begin{tikzpicture}
	\begin{axis}[
		axis x line=center,
		axis y line=center,
		%xtick={-5,-4,...,5},
		%ytick={-5,-4,...,5},
		xlabel={%%%%XLABEL%%%%},
		ylabel={%%%%YLABEL%%%%},
		xlabel style={below right},
		ylabel style={above left},
		xmin=0,
		%xmax=5.5,
		ymin=0,
		%ymax=5.5,
		%%%%ADDITIONAL%%PARAMETERS%%%%
		samples=50,
		]
		%%%ADD%%PLOTS%%%
	\end{axis}
\end{tikzpicture}
)xxx" };

	const std::string tikz_plot{
		R"xxx(\addplot+ [
		smooth,
		%%%%PLOT%%ATTRIBUTES%%%%
		] coordinates {
			%%%%COORDINATES%%%%
		};
		%%%%LEGEND%%ENTRY%%%%
		%%%ADD%%PLOTS%%%)xxx"
	};

	std::string diagram = tikz_template;

	diagram = std::regex_replace(diagram, std::regex("%%%%YLABEL%%%%"), y_label);
	diagram = std::regex_replace(diagram, std::regex("%%%%XLABEL%%%%"), x_label);

	for (const auto& graph : graphs) {

		const std::string tikz_values = std::accumulate(
			graph.value_pairs.begin(),
			graph.value_pairs.end(),
			std::string(""),
			[&](const std::string& acc, const auto& pair) {
				return acc + " (" + pair.first + ", " + pair.second + ")";
			});

		diagram = std::regex_replace(diagram, std::regex("%%%ADD%%PLOTS%%%"), tikz_plot);

		//diagram = std::regex_replace(diagram, std::regex("%%%%PLOT%%ATTRIBUTES%%%%"), graph.additional_attributes);
		diagram = std::regex_replace(diagram, std::regex("%%%%LEGEND%%ENTRY%%%%"), std::string("\\addlegendentry{") + graph.legend_entry + "}");

		diagram = std::regex_replace(diagram, std::regex("%%%%COORDINATES%%%%"), tikz_values);

	}
	//diagram = std::regex_replace(diagram, std::regex("%%%ADD%%PLOTS%%%"), tikz_plot);
	return diagram;
}

void parse_series_of_logs(const std::vector<uint64_t>& values_for_t, const std::vector<uint64_t>& values_for_m, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	for (auto t : values_for_t) {
		for (auto model_size : values_for_m) {
			auto p = std::filesystem::path(".");
			p = p / ".." / ".." / "..";
			p = p / "results" / std::to_string(model_size) / std::to_string(t);
			standard_logger()->debug(p.string());

			auto prism_log = file_to_string(p / "log.npptxt");
			auto time_file = file_to_string(p / "time.npptxt");

			all_files_loaded[t].insert(std::make_pair(model_size, PrismLogExtractor(prism_log, time_file)));
		}
	}
}

void safe_diagram_to_file(const std::string& diagram, const std::filesystem::path& destination) {

	std::ofstream diagram_file;

	diagram_file.open(destination.string());

	diagram_file << diagram;
}


void generate_m3_diagram(const std::filesystem::path& LATEX_DESTINATION, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	// diagram for all models 3 .. 7
// for every tested t a curve model mapped to time consuption.

	std::vector<uint64_t> m3_values_for_t;
	for (uint64_t i = 1000; i < 50'001; i += 1000) {
		m3_values_for_t.push_back(i);
	}
	m3_values_for_t.push_back(100'000);
	m3_values_for_t.push_back(150'000);
	m3_values_for_t.push_back(200'000);

	const uint64_t model_size = 3;

	parse_series_of_logs(m3_values_for_t, std::vector<uint64_t>{model_size}, all_files_loaded);

	std::vector<graph> graphs;

	graphs.emplace_back();

	std::transform( // t -> total time
		m3_values_for_t.begin(),
		m3_values_for_t.end(),
		std::back_inserter(graphs[0].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t /*/ 10'00.0*/), std::to_string(all_files_loaded[t][model_size].total_time_seconds)); }
	);

	graphs[0].legend_entry = labels::TOTAL_TIME_DIV_S;

	graphs.emplace_back();
	std::transform( // t-> constructed model size
		m3_values_for_t.begin(),
		m3_values_for_t.end(),
		std::back_inserter(graphs[1].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t/*/10'00.0*/), std::to_string((1.0 / 10'000) * all_files_loaded[t][model_size].states)); }
	);
	graphs[1].legend_entry = labels::COUNT_STATES_DIV_10000;


	auto diagram = create_diagram(graphs,"t /s", "N=3: total time, \\#states");

	safe_diagram_to_file(diagram, LATEX_DESTINATION / "diagram_m3_total_time_in_t.tex");

	std::ofstream diagram_m3_var_t;

	diagram_m3_var_t.open((LATEX_DESTINATION / "diagram_m3_total_time_in_t.tex").string());

	diagram_m3_var_t << diagram;

	//standard_logger()->debug(all_files_loaded[9000][3].prism_log);

}
void generate_m4_diagram(const std::filesystem::path& LATEX_DESTINATION, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	// diagram for all models 3 .. 7
// for every tested t a curve model mapped to time consuption.

	std::vector<uint64_t> values_for_t;
	for (uint64_t i = 1000; i < 12'001; i += 1000) {
		values_for_t.push_back(i);
	}

	uint64_t model_size = 4;

	parse_series_of_logs(values_for_t, std::vector<uint64_t>{model_size}, all_files_loaded);

	std::vector<graph> graphs;

	graphs.emplace_back();

	std::transform( // t -> total time
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[0].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string(all_files_loaded[t][model_size].total_time_seconds)); }
	);
	graphs[0].legend_entry = labels::TOTAL_TIME_DIV_S;

	graphs.emplace_back();
	std::transform( // t-> constructed model size
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[1].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string((1.0 / 10'000) * all_files_loaded[t][model_size].states)); }
	);
	graphs[1].legend_entry = labels::COUNT_STATES_DIV_10000;



	auto diagram = create_diagram(graphs, "t", "N=4: total time, \\#states");
	
	safe_diagram_to_file(diagram, LATEX_DESTINATION / "diagram_m4_total_time_in_t.tex");
	
}

void generate_m5_diagram(const std::filesystem::path& LATEX_DESTINATION, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	// diagram for all models 3 .. 7
// for every tested t a curve model mapped to time consuption.

	std::vector<uint64_t> values_for_t;
	for (uint64_t i = 100; i < 1'301; i += 100) {
		values_for_t.push_back(i);
	}

	uint64_t model_size = 5;

	parse_series_of_logs(values_for_t, std::vector<uint64_t>{model_size}, all_files_loaded);

	std::vector<graph> graphs;

	graphs.emplace_back();

	std::transform( // t -> total time
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[0].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string(all_files_loaded[t][model_size].total_time_seconds)); }
	);
	graphs[0].legend_entry = labels::TOTAL_TIME_DIV_S;

	graphs.emplace_back();
	std::transform( // t-> constructed model size
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[1].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string((1.0 / 10'000) * all_files_loaded[t][model_size].states)); }
	);
	graphs[1].legend_entry = labels::COUNT_STATES_DIV_10000;



	auto diagram = create_diagram(graphs, "t", "N=5: total time, \\#states");

	safe_diagram_to_file(diagram, LATEX_DESTINATION / "diagram_m5_total_time_in_t.tex");

}

void generate_m6_diagram(const std::filesystem::path& LATEX_DESTINATION, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	// diagram for all models 3 .. 7
// for every tested t a curve model mapped to time consuption.

	std::vector<uint64_t> values_for_t;
	for (uint64_t i = 10; i < 411; i += 10) {
		values_for_t.push_back(i);
	}

	uint64_t model_size = 6;

	parse_series_of_logs(values_for_t, std::vector<uint64_t>{model_size}, all_files_loaded);

	std::vector<graph> graphs;

	graphs.emplace_back();

	std::transform( // t -> total time
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[0].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string(all_files_loaded[t][model_size].total_time_seconds)); }
	);
	graphs[0].legend_entry = labels::TOTAL_TIME_DIV_S;

	graphs.emplace_back();
	std::transform( // t-> constructed model size
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[1].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string((1.0 / 10'000) * all_files_loaded[t][model_size].states)); }
	);
	graphs[1].legend_entry = labels::COUNT_STATES_DIV_10000;



	auto diagram = create_diagram(graphs, "t", "N=6: total time, \\#states");

	safe_diagram_to_file(diagram, LATEX_DESTINATION / "diagram_m6_total_time_in_t.tex");

}

void generate_m7_diagram(const std::filesystem::path& LATEX_DESTINATION, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	// diagram for all models 3 .. 7
// for every tested t a curve model mapped to time consuption.

	std::vector<uint64_t> values_for_t;
	for (uint64_t i = 5; i < 71; i += 5) {
		values_for_t.push_back(i);
	}

	uint64_t model_size = 7;

	parse_series_of_logs(values_for_t, std::vector<uint64_t>{model_size}, all_files_loaded);

	std::vector<graph> graphs;

	graphs.emplace_back();

	std::transform( // t -> total time
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[0].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string(all_files_loaded[t][model_size].total_time_seconds/1000)); }
	);
	graphs[0].legend_entry = "total time / $$10^3$$s";

	graphs.emplace_back();
	std::transform( // t-> constructed model size
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[1].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string((1.0 / 10'000'000) * all_files_loaded[t][model_size].states)); }
	);
	graphs[1].legend_entry = labels::COUNT_STATES_DIV_10000000;



	auto diagram = create_diagram(graphs, "t", "N=7: total time, \\#states");

	safe_diagram_to_file(diagram, LATEX_DESTINATION / "diagram_m7_total_time_in_t.tex");

}

void generate_m8_diagram(const std::filesystem::path& LATEX_DESTINATION, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	// diagram for all models 3 .. 7
// for every tested t a curve model mapped to time consuption.

	std::vector<uint64_t> values_for_t;
	for (uint64_t i = 1; i < 14; i += 1) {
		values_for_t.push_back(i);
	}

	uint64_t model_size = 8;

	parse_series_of_logs(values_for_t, std::vector<uint64_t>{model_size}, all_files_loaded);

	std::vector<graph> graphs;

	graphs.emplace_back();

	std::transform( // t -> total time
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[0].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string(all_files_loaded[t][model_size].total_time_seconds/1000)); }
	);
	graphs[0].legend_entry = labels::TOTAL_TIME_DIV_1000S;

	graphs.emplace_back();
	std::transform( // t-> constructed model size
		values_for_t.begin(),
		values_for_t.end(),
		std::back_inserter(graphs[1].value_pairs),
		[&](const uint64_t& t) { return std::make_pair(std::to_string(t), std::to_string((1.0 / 10'000'000) * all_files_loaded[t][model_size].states)); }
	);
	graphs[1].legend_entry = labels::COUNT_STATES_DIV_10000000;



	auto diagram = create_diagram(graphs, "t", "N=8: total time, \\#states");

	safe_diagram_to_file(diagram, LATEX_DESTINATION / "diagram_m8_total_time_in_t.tex");

}


void generate_all_diagram(const std::filesystem::path& LATEX_DESTINATION, std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>>& all_files_loaded) {

	// diagram for all models 3 .. 7
// for every tested t a curve model mapped to time consuption.

	std::map<uint64_t, std::vector<uint64_t>> m_to_values_for_t;
	for (uint64_t i = 1000; i < 50'001; i += 1000) {
		m_to_values_for_t[3].push_back(i);
	}
	for (uint64_t i = 1000; i < 12'001; i += 1000) {
		m_to_values_for_t[4].push_back(i);
	}
	for (uint64_t i = 100; i < 1'301; i += 100) {
		m_to_values_for_t[5].push_back(i);
	}
	for (uint64_t i = 10; i < 411; i += 10) {
		m_to_values_for_t[6].push_back(i);
	}
	for (uint64_t i = 5; i < 71; i += 5) {
		m_to_values_for_t[7].push_back(i);
	}
	for (uint64_t i = 1; i < 14; i += 1) {
		m_to_values_for_t[8].push_back(i);
	}


	std::vector<graph> graphs;

	for (const auto& pair : m_to_values_for_t) {
		const auto& model_size = pair.first;
		const auto& values_for_t = pair.second;

		//parse_series_of_logs(values_for_t, std::vector<uint64_t>{model_size}, all_files_loaded); reading files skipped

		graphs.emplace_back();

		for (const auto& t : values_for_t) {
			const auto& count_states = all_files_loaded[t][model_size].states;
			const auto& time_sec = all_files_loaded[t][model_size].total_time_seconds;

			graphs.back().value_pairs.push_back(
				std::make_pair(
					std::to_string(count_states),
					std::to_string(time_sec))
			);

			graphs.back().legend_entry = std::string("N=") + std::to_string(model_size);
		}
	}

	auto diagram = create_diagram(graphs, "\\#states", "total time / s");

	safe_diagram_to_file(diagram, LATEX_DESTINATION / "diagram_all_total_time_in_states.tex");

}

int main()
{
	const auto LATEX_DESTINATION = std::filesystem::path("..\\..\\..\\..\\TACAS\\sections\\experiments");

	init_logger();

	standard_logger()->info(std::string("Current dir:   ") + std::filesystem::current_path().string());

	std::map</*t*/ uint64_t, std::map</*model size*/ uint64_t, PrismLogExtractor>> all_files_loaded;

	generate_m3_diagram(LATEX_DESTINATION, all_files_loaded);
	generate_m4_diagram(LATEX_DESTINATION, all_files_loaded);
	generate_m5_diagram(LATEX_DESTINATION, all_files_loaded);
	generate_m6_diagram(LATEX_DESTINATION, all_files_loaded);
	generate_m7_diagram(LATEX_DESTINATION, all_files_loaded);
	generate_m8_diagram(LATEX_DESTINATION, all_files_loaded);
	generate_all_diagram(LATEX_DESTINATION, all_files_loaded);


	return 0;

	/*
	for (int t = 1000; t < 15000; t += 1000) {
		for (int model_size = 3; model_size < 4; ++model_size) {
			auto p = std::filesystem::path(".");
			p = p / ".." / ".." / "..";
			p = p / "results" / std::to_string(model_size) / std::to_string(t);
			standard_logger()->debug(p.string());

			auto prism_log = file_to_string(p / "log.npptxt");
			auto time_file = file_to_string(p / "time.npptxt");

			all_files_loaded[t].insert(std::make_pair(model_size, PrismLogExtractor(prism_log, time_file)));
		}
	}
*/
}

