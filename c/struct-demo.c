#include <stdio.h>

#define CHAR_ARRAY_LEN 16

void show_struct_memory_layout(void);
void anonymous_inner_structit(void);
void show_nested_struct_type(void);

int main(void)
{
	show_struct_memory_layout();
	anonymous_inner_structit();
	show_nested_struct_type();
	return 0;
}

void anonymous_inner_structit(void)
{
	printf("%s:\n", __func__);
	/* initialize in declaration order or by keyword; leftover members are initialized to 0 */
	struct {
		int int_member;
		const char *str_member;
		double double_member;
	} var1 = {111, "anonymous_var1"},
	  var2 = {.str_member = "anonymous_var2", .int_member = 222};
	printf("\tvar1={%d, %s, %f}\n\tvar2={%d, %s, %f}\n",
		var1.int_member, var1.str_member, var1.double_member,
		var2.int_member, var2.str_member, var2.double_member);
}

struct struct_1 {
	int member1;
	long member2;
	char member3[CHAR_ARRAY_LEN];
};

void show_struct_memory_layout(void)
{
	printf("%s:\n", __func__);
	printf("\tstruct: size=0x%zx\n", sizeof(struct struct_1));
	printf("\tMembers are named in declaration order.\n");
	printf("\tmember1: offset=0x%lx, size=0x%zx\n",
		(unsigned long)&((struct struct_1*)0)->member1,
		sizeof(((struct struct_1*)0)->member1));
	printf("\tmember2: offset=0x%lx, size=0x%zx\n",
		(unsigned long)&((struct struct_1*)0)->member2,
		sizeof(((struct struct_1*)0)->member2));
	printf("\tmember3: offset=0x%lx, size=0x%zx\n",
		(unsigned long)&((struct struct_1*)0)->member3,
		sizeof(((struct struct_1*)0)->member3));
}

struct outer_struct {
	/* anonymous struct */
	struct {
		int anonymous_struct_member;
	};

	/* named (untagged) struct instance */
	struct {
		int named_struct_member;
	} named_var;

	/* tagged struct: defined in the global name space */
	struct inner_struct {
		int tagged_struct_member;
	} tagged_var;

	/* tagged "unnamed" struct -- bad coding style; should be placed out of this struct */
	struct inner_anonymous_struct {
		int tagged_anonymous_struct_member;
	};
};

void show_nested_struct_type(void)
{
	printf("%s:\n", __func__);
	printf("\tsizeof(outer_struct)=0x%zx\n", sizeof(struct outer_struct));
	printf("\tsizeof(tagged_struct)=0x%zx, sizeof(named instance)=0x%zx, " \
		"sizeof(tagged_anonymous_struct)=0x%zx\n",
		sizeof(struct inner_struct),
		sizeof(((struct outer_struct*)0)->named_var),
		sizeof(struct inner_anonymous_struct));

	struct outer_struct out;

	/* untagged unnamed: direct access */
	out.anonymous_struct_member = 1;

	/* error: cannot directly access field of named nested struct */
	/* invalid: out.named_struct_member = 2; */
	out.named_var.named_struct_member = 2;
	/* invalid: out.tagged_struct_member = 3; */
	out.tagged_var.tagged_struct_member = 3;

	/* error: tagged unamed struct is just a declaration, not really a member */
	/* invalid: not such process */
	/* out.tagged_anonymous_struct_member = 4; */

	/* however, all tagged struct can be used as a type */
	struct inner_struct inner;
	struct inner_anonymous_struct inner_anonymous;
	inner.tagged_struct_member = inner_anonymous.tagged_anonymous_struct_member = -1;

	printf("\tout.bytes (hex):");
	for (size_t i = 0; i < sizeof(out); i++)
		printf(" %x", *((const char *)&out + i));
	printf("\n");
}
