#include <stdio.h>

void show_union_layout(void);
void show_union_in_struct(void);

int main(void)
{
	show_union_layout();
	show_union_in_struct();
	return 0;
}

void show_union_layout(void)
{
	union {
		struct {
			int member_0_0;
		};
		struct {
			long member_1_0;
			int member_1_1;
		};
	} u;

	printf("%s:\n", __func__);
	printf("\tsizeof(union)=0x%zx\n", sizeof(u));

	printf("\tcase 1:\n");
	printf("\t\tmember_0_0:{offset=0x%lx, size=0x%zx}\n",
		(unsigned long)&u.member_0_0 - (unsigned long)&u, sizeof(u.member_0_0));

	printf("\tcase 2:\n");
	printf("\t\tmember_1_0:{offset=0x%lx, size=0x%zx}\n" \
		"\t\tmember_1_1:{offset=0x%lx, size=0x%zx}\n",
		(unsigned long)&u.member_1_0 - (unsigned long)&u, sizeof(u.member_1_0),
		(unsigned long)&u.member_1_1 - (unsigned long)&u, sizeof(u.member_1_1)
	);
}

struct outer_struct {
	/* anonymous union */
	union {
		short f1;
	};

	/* named (untagged) union instance */
	union {
		unsigned int f2;
	} u1;

	/* tagged union: defined in the global name space */
	union inner_union {
		long long f3;
	} u2;

	/* tagged "unnamed" union -- bad coding style; should be placed out of this struct */
	union inner_anonymous_union {
		long long f4;
	};
};

void show_union_in_struct(void)
{
	printf("%s:\n", __func__);
	printf("\tsizeof(outer_struct)=0x%zx\n", sizeof(struct outer_struct));
	printf("\tsizeof(tagged_union)=0x%zx, sizeof(named instance)=0x%zx, " \
		"sizeof(tagged_anonymous_union)=0x%zx\n",
		sizeof(union inner_union),
		sizeof(((struct outer_struct*)0)->u1),
		sizeof(union inner_anonymous_union));

	struct outer_struct out;

	/* error: tagged unamed struct is just a declaration, not really a member */
	/* invalid: not such process */
	/* out.f4 = 4; */

	/* error: cannot directly access field of named nested struct */
	/* invalid: out.f3 = 3; */
	out.u2.f3 = 0x3UL << 56;
	/* invalid: out.f2 = 2; */
	out.u1.f2 = 0x2U << 24;

	/* untagged unnamed: direct access */
	out.f1 |= 1;

	/* however, all tagged struct can be used as a type */
	union inner_union inner;
	union inner_anonymous_union inner_anonymous;
	inner.f3 = inner_anonymous.f4 = -1;

	printf("\tout.bytes (hex):");
	for (size_t i = 0; i < sizeof(out); i++)
		printf(" %x", *((const char *)&out + i));
	printf("\n");
}
