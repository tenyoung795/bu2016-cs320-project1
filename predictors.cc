#include <functional>
#include <type_traits>

//#define MEMCHECK
#ifdef MEMCHECK
#include <valgrind/memcheck.h>
#endif

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
constexpr counter_table<CounterBitwidth, NumEntries>::counter_table(): bits()
{
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
counter_table<CounterBitwidth, NumEntries>::counter_table(all_ones_t)
{
	bits.fill(UCHAR_MAX);
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
unsigned int counter_table<CounterBitwidth, NumEntries>::operator[](pc_t pc) const
{
	auto index = pc % NumEntries;
	auto block_pos = index_to_block(index);
	auto block = bits[index_to_block_pos(index)];
	return (block & (MAX_ENTRY << block_pos)) >> block_pos;
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
auto counter_table<CounterBitwidth, NumEntries>::operator[](pc_t pc) -> reference
{
	return reference(bits, pc % NumEntries);
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
constexpr std::size_t counter_table<CounterBitwidth, NumEntries>::index_to_block(std::size_t index)
{
	return index * COUNTER_BITWIDTH / CHAR_BIT;
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
constexpr unsigned int counter_table<CounterBitwidth, NumEntries>::index_to_block_pos(std::size_t index)
{
	return index * COUNTER_BITWIDTH % CHAR_BIT;
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
counter_table<CounterBitwidth, NumEntries>::reference::reference(array &arr, std::size_t index):
	block(arr[index_to_block(index)]),
	block_pos(index_to_block_pos(index))
{
	#ifdef MEMCHECK
	VALGRIND_CHECK_VALUE_IS_DEFINED(arr);
	#endif
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
counter_table<CounterBitwidth, NumEntries>::reference::operator unsigned int() const
{
	return (block & (MAX_ENTRY << block_pos)) >> block_pos;
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
bool counter_table<CounterBitwidth, NumEntries>::reference::mode() const
{
	return block & (MIDPOINT << block_pos);
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
template <class Function>
void counter_table<CounterBitwidth, NumEntries>::reference::transform(Function f)
{
	static_assert(std::is_convertible<Function, std::function<unsigned int(unsigned int)>>::value,
		"Argument function must take in and return an unsigned int");
	auto entry = operator unsigned int();
	block = (block & ~(MAX_ENTRY << block_pos)) | (f(entry) << block_pos);
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
void counter_table<CounterBitwidth, NumEntries>::reference::operator++()
{
	transform([](unsigned int entry) { return entry == MAX_ENTRY? MAX_ENTRY : entry + 1u; }); 
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
void counter_table<CounterBitwidth, NumEntries>::reference::operator--()
{
	transform([](unsigned int entry) { return entry? entry - 1u : 0u; }); 
}

template <counter_bitwidth CounterBitwidth, std::size_t NumEntries>
bool bimodal<CounterBitwidth, NumEntries>::predict(pc_t pc, bool taken)
{
	auto counter = table[pc];
	bool correct = counter.mode() == taken;
	if (taken)
		++counter;
	else
		--counter;
	return correct;
}

template <std::size_t Bitwidth>
constexpr global_history_register<Bitwidth>::global_history_register():
	bits(ULLONG_MAX)
{
}

template <std::size_t Bitwidth>
pc_t global_history_register<Bitwidth>::operator^(pc_t pc) const
{
	return pc ^ bits.to_ullong();
}

template <std::size_t Bitwidth>
pc_t operator^(pc_t pc, const global_history_register<Bitwidth> &history)
{
	return history ^ pc;
}

template <std::size_t Bitwidth>
void global_history_register<Bitwidth>::operator<<(bool b)
{
	bits <<= 1;
	bits.set(0, b);
}

template <std::size_t HistoryBitwidth>
bool gshare<HistoryBitwidth>::predict(pc_t pc, bool taken)
{
	auto correct = pred.predict(pc ^ global_history, taken);
	global_history << taken;
	return correct;
}
