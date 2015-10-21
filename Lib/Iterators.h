#pragma once

#include <iterator>



template <typename Container, typename ConstIterator>
typename Container::iterator remove_constness(Container &c, ConstIterator it)
{
	return c.erase(it, it);
}

////////////////////////////////////////////////////////////////////////////////////////////////////


template<typename Container>
inline
typename Container::iterator prev_cyclic(typename Container::iterator it,
                                         Container &c)
{
	static_assert((std::is_base_of<std::bidirectional_iterator_tag,
	               typename std::iterator_traits<Container::iterator>::iterator_category>::value),
	              "prev_cyclic requires bidirectional iterator");

	if ( it == c.begin() )
		it = c.end();
	std::advance(it, -1);
	return it;
}



template<typename Container>
inline
typename Container::const_iterator prev_cyclic(typename Container::const_iterator it,
                                               Container &c)
{
	static_assert((std::is_base_of<std::bidirectional_iterator_tag,
	               typename std::iterator_traits<Container::const_iterator>::iterator_category>::value),
	              "prev_cyclic requires bidirectional iterator");

	if ( it == c.begin() )
		it = c.end();
	std::advance(it, -1);
	return it;
}



template<typename Container>
inline
typename Container::const_iterator prev_cyclic(typename Container::const_iterator it,
                                               Container const &c)
{
	static_assert((std::is_base_of<std::bidirectional_iterator_tag,
	               typename std::iterator_traits<Container::const_iterator>::iterator_category>::value),
	              "prev_cyclic requires bidirectional iterator");

	if ( it == c.begin() )
		it = c.end();
	std::advance(it, -1);
	return it;
}



template<typename Container>
inline
typename Container::iterator next_cyclic(typename Container::iterator it,
                                         Container &c)
{
	static_assert((std::is_base_of<std::forward_iterator_tag,
	               typename std::iterator_traits<Container::iterator>::iterator_category>::value),
	              "next_cyclic requires forward iterator");

	std::advance(it, 1);
	if ( it == c.end() )
		it = c.begin();
	return it;
}



template<typename Container>
inline
typename Container::const_iterator next_cyclic(typename Container::const_iterator it,
                                               Container &c)
{
	static_assert((std::is_base_of<std::forward_iterator_tag,
	               typename std::iterator_traits<Container::const_iterator>::iterator_category>::value),
	              "next_cyclic requires forward iterator");

	std::advance(it, 1);
	if ( it == c.end() )
		it = c.begin();
	return it;
}



template<typename Container>
inline
typename Container::const_iterator next_cyclic(typename Container::const_iterator it,
                                               Container const &c)
{
	static_assert((std::is_base_of<std::forward_iterator_tag,
	               typename std::iterator_traits<Container::const_iterator>::iterator_category>::value),
	              "next_cyclic requires forward iterator");

	std::advance(it, 1);
	if ( it == c.end() )
		it = c.begin();
	return it;
}
