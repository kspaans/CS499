#include <types.h>
#include <panic.h>
#include <string.h>
#include <lib.h>

size_t iov_length(const struct iovec *iov, int iovlen) {
	size_t ret = 0;
	for (int i = 0; i < iovlen; ++i)
		ret += iov[i].iov_len;
	return ret;
}

struct iovec_iter {
	const struct iovec *iov;
	int iovlen;
	char *buf;
	size_t len;
};

static void iov_advance(struct iovec_iter *iter) {
	while (!iter->len && iter->iovlen) {
		iter->buf = iter->iov->iov_base;
		iter->len = iter->iov->iov_len;
		iter->iov++;
		iter->iovlen--;
	}
}

void iov_copy(const struct iovec *srciov, int srclen, const struct iovec *dstiov, int dstlen) {
	struct iovec_iter src = { srciov, srclen, NULL, 0 };
	struct iovec_iter dst = { dstiov, dstlen, NULL, 0 };

	iov_advance(&src);
	iov_advance(&dst);

	while (src.len) {
		if (!dst.len)
			panic("iov_copy: buffer overflow %d %d", iov_length(srciov, srclen), iov_length(dstiov, dstlen));

		size_t len = src.len < dst.len ? src.len : dst.len;
		memcpy(dst.buf, src.buf, len);

		src.buf += len;
		dst.buf += len;
		src.len -= len;
		dst.len -= len;

		iov_advance(&src);
		iov_advance(&dst);
	}
}
