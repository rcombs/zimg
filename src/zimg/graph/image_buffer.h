#pragma once

#ifndef ZIMG_GRAPH_IMAGE_BUFFER_H_
#define ZIMG_GRAPH_IMAGE_BUFFER_H_

#include <cstddef>
#include <limits>
#include <type_traits>
#include "common/propagate_const.h"

namespace zimg {;
namespace graph {;

// Special mask value with all bits set.
const unsigned BUFFER_MAX = (unsigned)-1;

/**
 * Circular image buffer.
 *
 * @tparam T held type
 */
template <class T>
class ImageBuffer {
	// Use a void pointer to ensure all instantiations are layout compatible.
	typename propagate_const<T, void>::type *m_data;
	ptrdiff_t m_stride;
	unsigned m_mask;

	T *at_line(unsigned i) const
	{
		auto *byte_ptr = static_cast<typename propagate_const<T, char>::type *>(m_data);
		return reinterpret_cast<T *>(byte_ptr + static_cast<ptrdiff_t>(i & m_mask) * m_stride);
	}
public:
	/**
	 * Default construct ImageBuffer, creating a null buffer.
	 */
	ImageBuffer() :
		m_data{},
		m_stride{},
		m_mask{}
	{
	}

	/**
	 * Construct an ImageBuffer from pointer and mask.
	 *
	 * @param data pointer to base of buffer
	 * @param stride buffer stride in bytes, may be negative
	 * @param mask row index mask
	 */
	ImageBuffer(T *data, ptrdiff_t stride, unsigned mask) :
		m_data{ data },
		m_stride{ stride },
		m_mask{ mask }
	{
		static_assert(std::is_standard_layout<ImageBuffer>::value, "layout error");
	}

	/**
	 * Construct an ImageBuffer from a buffer of an implicitly convertible type.
	 *
	 * @tparam U type convertible to T
	 * @param other original buffer
	 */
	template <class U>
	ImageBuffer(const ImageBuffer<U> &other, typename std::enable_if<std::is_convertible<U *, T *>::value>::type * = nullptr) :
		m_data{ other.data() },
		m_stride{ other.stride() },
		m_mask{ other.mask() }
	{
	}

	/**
	 * Get the underlying data pointer.
	 *
	 * @return pointer
	 */
	T *data() const
	{
		return at_line(0);
	}

	/**
	 * Get the underlying image stride.
	 *
	 * @return stride
	 */
	ptrdiff_t stride() const
	{
		return m_stride;
	}

	/**
	 * Get the underlying row mask.
	 *
	 * @return mask
	 */
	unsigned mask() const
	{
		return m_mask;
	}

	/**
	 * Get pointer to scanline.
	 *
	 * @param i row index
	 * @return pointer to beginning of line
	 */
	T *operator[](unsigned i) const
	{
		return at_line(i);
	}

	/**
	 * Cast buffer to another type.
	 *
	 * @tparam U type convertible from T by static_cast
	 * @return buffer as other type
	 */
	template <class U>
	const ImageBuffer<U> &static_buffer_cast() const
	{
		static_assert(std::is_standard_layout<decltype(static_cast<U *>(static_cast<T *>(m_data)))>::value,
		              "type not convertible by static_cast");

		// Break strict aliasing to avoid unnecessary object copies.
		return *reinterpret_cast<const ImageBuffer<U> *>(this);
	}
};

/**
 * Wrapper around array of three {@link ImageBuffer}.
 *
 * @tparam T buffer held type
 */
template <class T>
class ColorImageBuffer {
	ImageBuffer<T> m_buffer[3];
public:
	/**
	 * Default construct ColorImageBuffer, creating an array of null buffers.
	 */
	ColorImageBuffer() :
		m_buffer{}
	{
	}

	/**
	 * Construct a ColorImageBuffer from individual buffers.
	 *
	 * @param buf1 first channel
	 * @param buf2 second channel
	 * @param buf3 third channel
	 */
	ColorImageBuffer(const ImageBuffer<T> &buf1, const ImageBuffer<T> &buf2, const ImageBuffer<T> &buf3) :
		m_buffer{ buf1, buf2, buf3 }
	{
	}

	/**
	 * Construct a ColorImageBuffer from an implicitly convertible type.
	 *
	 * @tparam U type convertible to T
	 * @param other original buffer
	 */
	template <class U>
	ColorImageBuffer(const ColorImageBuffer<U> &other, typename std::enable_if<std::is_convertible<U *, T *>::value>::type * = nullptr) :
		m_buffer{ other[0], other[1], other[2] }
	{
	}

	/**
	 * Implicit conversion to array of {@link ImageBuffer}.
	 *
	 * @return pointer
	 */
	operator const ImageBuffer<T> *() const
	{
		return m_buffer;
	}

	/**
	 * @see operator const ImageBuffer<T> *
	 */
	operator ImageBuffer<T> *()
	{
		return m_buffer;
	}

	/**
	* Cast buffer to another type.
	*
	* @tparam U type convertible from T by static_cast
	* @return buffer as other type
	*/
	template <class U>
	const ColorImageBuffer<U> &static_buffer_cast() const
	{
		static_assert(std::is_standard_layout<decltype(m_buffer->template static_buffer_cast<U>())>::value,
		              "type not convertible by static_cast");

		// Break strict aliasing to avoid unnecessary object copies.
		return *reinterpret_cast<const ColorImageBuffer<U> *>(this);
	}
};

/**
 * @see ImageBuffer::static_buffer_cast
 */
template <class U, class T>
const ImageBuffer<U> &static_buffer_cast(const ImageBuffer<T> &buf)
{
	return buf.template static_buffer_cast<U>();
}

/**
 * @see ImageBuffer::static_buffer_cast
 */
template <class U, class T>
const ImageBuffer<U> *static_buffer_cast(const ImageBuffer<T> *buf)
{
	return &static_buffer_cast<U>(*buf);
}

/**
 * @see ColorImageBuffer::static_buffer_cast
 */
template <class U, class T>
const ColorImageBuffer<U> &static_buffer_cast(const ColorImageBuffer<T> &buf)
{
	return buf.template static_buffer_cast<U>();
}

/**
 * Convert a line count to a buffer mask.
 *
 * @param count line count, may be {@link BUFFER_MAX}
 * @return mask, may be {@link BUFFER_MAX}
 */
inline unsigned select_zimg_buffer_mask(unsigned count)
{
	const unsigned UINT_BITS = std::numeric_limits<unsigned>::digits;

	if (count != 0 && ((count - 1) & (1U << (UINT_BITS - 1))))
		return BUFFER_MAX;

	for (unsigned i = UINT_BITS - 1; i != 0; --i) {
		if ((count - 1) & (1U << (i - 1)))
			return (1U << i) - 1;
	}

	return 0;
}

} // namespace graph
} // namespace zimg

#endif // ZIMG_GRAPH_ZTYPES_H_
