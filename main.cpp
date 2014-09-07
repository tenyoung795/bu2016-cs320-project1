#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "metatemplates.h"
#include "predictors.h"

using pred_ptr = std::unique_ptr<predictor>;
using pred_map = std::vector<std::pair<pred_ptr, int>>;

template <counter_bitwidth HistoryBitwidth, class EntrySizeSequence>
struct bimodal_type_list;

template <counter_bitwidth HistoryBitwidth>
struct bimodal_type_list<HistoryBitwidth, size_sequence<>>
{
	using type = type_list<>;
};

template <counter_bitwidth HistoryBitwidth, std::size_t Head, std::size_t... Tail>
struct bimodal_type_list<HistoryBitwidth, size_sequence<Head, Tail...>>	
{
	using type = typename type_list_cons<bimodal<HistoryBitwidth, Head>,
		typename bimodal_type_list<HistoryBitwidth, size_sequence<Tail...>>::type>::type;
};

template <class SizeSequence>
struct gshare_type_list;

template <>
struct gshare_type_list<size_sequence<>>
{
	using type = type_list<>;
};

template <std::size_t Head, std::size_t... Tail>
struct gshare_type_list<size_sequence<Head, Tail...>>	
{
	using type = typename type_list_cons<gshare<Head>,
		typename gshare_type_list<size_sequence<Tail...>>::type>::type;
};

template <class... Types>
struct set_pred_map_helper;

template <>
struct set_pred_map_helper<>
{
	static void emplace_back(pred_map &)
	{
	}
};

template <class Head, class... Tail>
struct set_pred_map_helper<Head, Tail...>
{
	static void emplace_back(pred_map &m)
	{
		m.emplace_back(pred_ptr(new Head), 0);
		set_pred_map_helper<Tail...>::emplace_back(m);
	}
};

template <class TypeList>
struct set_pred_map;

template <class... Types>
struct set_pred_map<type_list<Types...>>
{
	static void set(pred_map &m)
	{
		m.reserve(sizeof...(Types));
		set_pred_map_helper<Types...>::emplace_back(m);
		m.shrink_to_fit();
	}
};

template <std::size_t Index, class... TypeLists>
struct make_pred_maps_helper;

template <std::size_t Index>
struct make_pred_maps_helper<Index>
{
	template <class Array>
	static void set(Array &)
	{
	}
};

template <std::size_t Index, class Head, class... Tail>
struct make_pred_maps_helper<Index, Head, Tail...>
{
	template <class Array>
	static void set(Array &arr)
	{
		set_pred_map<Head>::set(arr[Index]);
		make_pred_maps_helper<Index + 1, Tail...>::set(arr);
	}
};

template <class... TypeLists>
struct make_pred_maps
{
	private:
	using array = std::array<pred_map, sizeof...(TypeLists)>;

	public:
	static array make()
	{
		array arr;
		make_pred_maps_helper<0, TypeLists...>::set(arr);
		return arr;
	}
};

int main(int argc, const char *const *argv)
{
	auto pname = argv[0];
	auto step_and_check_argc = [&]()
	{
		++argv;
		--argc;
		if (argc < 1)
		{
			std::cerr << "Usage: " << pname << " [-e] <input-filename> <output-filename>\n"
			"\t-e enable extra-credit predictor\n";
			std::exit(EXIT_FAILURE);
		}
	};
	step_and_check_argc();

	bool extra_enabled = false;
	if (!std::strcmp(*argv, "-e"))
	{
		extra_enabled = true;
		step_and_check_argc();
	}

	std::ifstream input_file(*argv);
	if (!input_file)
	{
		std::cerr << "Could not open " << *argv << " for input\n";
		return EXIT_FAILURE;
	}
	input_file.exceptions(std::ios_base::badbit);
	input_file >> std::setbase(0);
	step_and_check_argc();

	std::ofstream output_file(*argv, std::ios_base::trunc);
	if (!output_file)
	{
		std::cerr << "Could not open " << *argv << " for output\n";
		return EXIT_FAILURE;
	}
	output_file.exceptions(std::ios_base::failbit);

	using bimodal_entry_sizes = size_sequence<8, 16, 32, 128, 256, 512, 1024>;
	auto pred_maps = make_pred_maps<
		type_list<always_taken>,
		type_list<always_not_taken>,
		bimodal_type_list<counter_bitwidth::ONE, bimodal_entry_sizes>::type,
		bimodal_type_list<counter_bitwidth::TWO, bimodal_entry_sizes>::type,
		gshare_type_list<size_range<2, 11>::type>::type,
		type_list<tournament>,
		type_list<yags>
	>::make(); 

	pc_t pc;
	std::string taken_str;
	std::size_t branch = 0;
	for (; input_file >> pc; branch++)
	{
		bool taken;
		input_file >> taken_str;
		if (taken_str == "NT")
			taken = false;
		else if (taken_str == "T")
			taken = true;
		else
		{
			std::cerr << argv[1] << ':' << (branch + 1) << ' ' << taken_str << " does not match either NT or T\n";
			return EXIT_FAILURE;
		}

		auto last = std::prev(pred_maps.end());
		auto run_predictor = [&](decltype(pred_maps)::iterator iter)
		{
			for (auto &p : *iter)
				if (p.first->predict(pc, taken))
					p.second++;
		};

		for (auto iter = pred_maps.begin(); iter != last; ++iter)
			run_predictor(iter);

		if (extra_enabled)
			run_predictor(last);
	}
	if (!input_file.eof())
	{
		std::cerr << argv[1] << ':' << (branch + 1) << ' ' << "Expected 32-bit PC\n";
		return EXIT_FAILURE;
	}

	auto last = std::prev(pred_maps.cend());
	auto run_printer = [&](decltype(pred_maps)::const_iterator outer_iter)
	{
		auto last = std::prev(outer_iter->cend());
		for (auto iter = outer_iter->cbegin(); iter != last; ++iter)
		{
			output_file << iter->second << ',' << branch << "; ";
		}
		output_file << last->second << ',' << branch << ";\n\n";
	};
	for (auto iter = pred_maps.cbegin(); iter != last; ++iter)
		run_printer(iter);
	if (extra_enabled)
		run_printer(last);

	return 0;
}
