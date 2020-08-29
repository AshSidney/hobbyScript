#include <gtest/gtest.h>
#include <CppScript/Visitor.h>

using namespace CppScript;

using TestVisitables = TypePack<class A, class B, class C>;
using TestVisitor = TestVisitables::apply<Visitor>;

class VisitorAll : public TestVisitor
{
public:
	void visit(const A& visitable) override;
	void visit(A& visitable) override;
	void visit(const B& visitable) override;
	void visit(B& visitable) override;
	void visit(const C& visitable) override;
	void visit(C& visitable) override;

	int x{ 0 };
	float y{ 0 };
	bool z{ false };
};

class A : public VisitableBase<A, TestVisitor>
{
public:
	A() = default;
	A(bool val) : z(val)
	{}
	virtual ~A() = default;

	bool z{ false };
};

class B : public Visitable<B, A, TestVisitor>
{
public:
	B(int val) : x(val)
	{}

	int x;
};

class C : public Visitable<C, A, TestVisitor>
{
public:
	C(float val) : y(val)
	{}

	float y;
};

void VisitorAll::visit(const A& visitable)
{
	z = visitable.z;
}
void VisitorAll::visit(A& visitable)
{
	visitable.z = !z;
}
void VisitorAll::visit(const B& visitable)
{
	x = visitable.x;
}
void VisitorAll::visit(B& visitable)
{
	visitable.x += 2;
}
void VisitorAll::visit(const C& visitable)
{
	y = visitable.y;
}
void VisitorAll::visit(C& visitable)
{
	visitable.y += 2.3f;
}

TEST(VisitorTestCase, VisitorAll)
{
	A a{ true };
	const A& ar{ a };
	B b{ 4 };
	const B& br{ b };
	C c{ 6.7f };
	const C& cr{ c };

	VisitorAll vis;
	ar.accept(vis);
	br.accept(vis);
	cr.accept(vis);

	EXPECT_TRUE(vis.z);
	EXPECT_EQ(vis.x, 4);
	EXPECT_EQ(vis.y, 6.7f);

	a.accept(vis);
	b.accept(vis);
	c.accept(vis);

	EXPECT_FALSE(a.z);
	EXPECT_EQ(b.x, 6);
	EXPECT_EQ(c.y, 9.0f);
}


using TestVisitorDefault = TestVisitables::apply<VisitorDefault>;

class VisitorC : public TestVisitorDefault
{
public:
	void visit(const C& visitable) override
	{
		y = visitable.y * 2;
	}
	void visit(C& visitable) override
	{
		visitable.y = y;
	}

	float y{ 0 };
};

TEST(VisitorTestCase, VisitorDefault)
{
	B b{ 4 };
	const B& br{ b };
	C c{ 6.7f };
	const C& cr{ c };

	VisitorC vis;
	br.accept(vis);
	cr.accept(vis);

	EXPECT_EQ(vis.y, 13.4f);

	b.accept(vis);
	c.accept(vis);

	EXPECT_EQ(b.x, 4);
	EXPECT_EQ(c.y, 13.4f);
}


using TestVisitorFailing = TestVisitables::apply<VisitorFailing>;

class VisitorB : public TestVisitorFailing
{
public:
	void visit(const B& visitable) override
	{
		x = visitable.x + 7;
	}
	void visit(B& visitable) override
	{
		visitable.x = x - 5;
	}

	int x{ 0 };
};

TEST(VisitorTestCase, VisitorFailing)
{
	B b{ 4 };
	const B& br{ b };
	C c{ 6.7f };
	const C& cr{ c };

	VisitorB vis;
	br.accept(vis);
	EXPECT_THROW(cr.accept(vis), UnexpectedVisit<C>);

	EXPECT_EQ(vis.x, 11);

	b.accept(vis);
	EXPECT_THROW(c.accept(vis), UnexpectedVisit<C>);

	EXPECT_EQ(b.x, 6);
}
