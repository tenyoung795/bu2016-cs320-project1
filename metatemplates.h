#ifndef METATEMPLATES_H
#define METATEMPLATES_H

template <std::size_t...>
struct size_sequence
{
};

template <std::size_t, class SizeSequence>
struct size_sequence_cons;

template <std::size_t Head, std::size_t... Tail>
struct size_sequence_cons<Head, size_sequence<Tail...>>
{
	using type = size_sequence<Head, Tail...>;
};

template <std::size_t Begin, std::size_t End, class = void>
struct size_range;

template <std::size_t End>
struct size_range<End, End>
{
	using type = size_sequence<>;
};

template <std::size_t Begin, std::size_t End>
struct size_range<Begin, End, typename std::enable_if<Begin < End>::type>	
{
	using type = typename size_sequence_cons<Begin,
		typename size_range<Begin + 1, End>::type>::type;
};

template <class... Types>
struct type_list
{
};

template <class Head, class TypeList>
struct type_list_cons;

template <class Head, class... Tail>
struct type_list_cons<Head, type_list<Tail...>>
{
	using type = type_list<Head, Tail...>;
};

#endif
