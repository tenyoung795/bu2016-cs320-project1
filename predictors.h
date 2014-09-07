#ifndef PREDICTORS_H
#define PREDICTORS_H

#include <array>
#include <bitset>
#include <climits>
#include <cstddef>
#include <cstdint>

using pc_t = uint32_t;

class predictor
{
	public:
	/* Predicts whether the branch is taken.
	 * Returns whether this was correct. */
	virtual bool predict(pc_t pc, bool taken) = 0;
};

class always_taken : public predictor
{
	public:
	bool predict(pc_t pc, bool taken);
};

class always_not_taken : public predictor
{
	public:
	bool predict(pc_t pc, bool taken);
};

enum class counter_bitwidth : unsigned int
{
	ONE = 1,
	TWO = 2
};

constexpr struct all_ones_t {} all_ones{};

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
class counter_table
{	
	private:	
	static constexpr auto COUNTER_BITWIDTH = static_cast<unsigned int>(CounterBitwidth);
	static constexpr auto MAX_ENTRY = (1u << COUNTER_BITWIDTH) - 1u;
	static constexpr auto MIDPOINT = 1u << (COUNTER_BITWIDTH - 1u);

	static constexpr std::size_t index_to_block(std::size_t);
	static constexpr unsigned int index_to_block_pos(std::size_t);

	using array = std::array<unsigned char, (COUNTER_BITWIDTH * NumEntries) / CHAR_BIT>;
	class reference
	{
		public:
		reference(array &, std::size_t);
		operator unsigned int() const;
		bool mode() const;
		void operator++();
		void operator--();

		private:
		typename array::reference block;
		unsigned int block_pos;

		template <class Function>
		void transform(Function f);
	};		
	array bits;

	public:
	constexpr counter_table();
	counter_table(all_ones_t);

	unsigned int operator[](pc_t) const;
	reference operator[](pc_t);
};

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
class bimodal : public predictor
{
	public:	
	bool predict(pc_t pc, bool taken);

	private:
	counter_table<CounterBitwidth, NumEntries> table;
};

template <std::size_t Bitwidth>
class global_history_register
{
	public:
	constexpr global_history_register();
	pc_t operator^(pc_t) const;	
	void operator<<(bool b);
	
	private:
	std::bitset<Bitwidth> bits;
};

template <std::size_t Bitwidth>
pc_t operator^(pc_t, const global_history_register<Bitwidth> &);

template <std::size_t HistoryBitwidth>
class gshare : public predictor
{
	public:	
	bool predict(pc_t pc, bool taken);

	private:
	static constexpr auto NUM_ENTRIES = 1024u;
	bimodal<counter_bitwidth::TWO, NUM_ENTRIES> pred;
	global_history_register<HistoryBitwidth> global_history;
};

class tournament : public predictor
{
	public:
	tournament();
	bool predict(pc_t pc, bool taken);

	private:
	static constexpr auto NUM_ENTRIES = 1024u;
	static constexpr auto HISTORY_BITWIDTH = 10u;
	gshare<HISTORY_BITWIDTH> gshare_pred;
	bimodal<counter_bitwidth::TWO, NUM_ENTRIES> bimodal_pred;
	counter_table<counter_bitwidth::TWO, NUM_ENTRIES> selector_table;
};

class yags : public predictor
{
	public:
	yags();
	bool predict(pc_t pc, bool taken);

	private:
	class cache
	{	
		private:
		class entry
		{
			public:
			constexpr entry(bool mode = false);
			constexpr bool hit(pc_t pc) const;
			constexpr bool mode() const;
			void set_tag(pc_t pc);
			void operator++();
			void operator--();

			private:
			static constexpr auto TAG_BITWIDTH = 6u;
			static constexpr auto TAG_BITMASK = (1u << TAG_BITWIDTH) - 1u;
			static constexpr auto COUNTER_BITWIDTH = 2u;
			static constexpr auto MAX_COUNTER = (1u << COUNTER_BITWIDTH) - 1u;
			static constexpr auto COUNTER_MODE_BITMASK = 1u << (COUNTER_BITWIDTH - 1u);

			unsigned int tag : TAG_BITWIDTH;
			unsigned int counter : COUNTER_BITWIDTH;
		};
		static constexpr auto NUM_ENTRIES = 256u;
		using arr = std::array<entry, NUM_ENTRIES>;
		using ref = arr::reference;
		using cref = arr::const_reference;
		arr entries;

		public:
		static constexpr struct mode_1_t {} mode_1{};

		constexpr cache() = default;
		cache(mode_1_t);

		ref operator[](pc_t);
		cref operator[](pc_t) const;
	};
	static constexpr auto NUM_SELECTORS = 1024u;
	static constexpr auto HISTORY_BITWIDTH = 8u;
	counter_table<counter_bitwidth::TWO, NUM_SELECTORS> selector_table;
	cache taken_cache, not_taken_cache;
	global_history_register<HISTORY_BITWIDTH> global_history;
};

#include "predictors.cc"

#endif
