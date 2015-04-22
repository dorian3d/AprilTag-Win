#include <cmath>
#include <util.h>

::testing::AssertionResult test_m4_near(const char* expr1, const char* expr2, const char* bounds_expr,
                                        const m4 &a, const m4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i)
        for(int j = 0; j < 4; ++j)
            if(fabs(a(i,j) - b(i,j)) > bounds)
                return ::testing::AssertionFailure()
                    << "The difference between\n"
                    <<  " " << a(i,j) << " = " << expr1 << "(" << i << "," << j << ")\n"
                    <<  " " << b(i,j) << " = " << expr2 << "(" << i << "," << j << ")\n"
                    << "is " << fabs(a(i,j) - b(i,j)) << "\n"
                    << "where\n"
                    << " " << expr1 << " =\n"
                    << a << "\n"
                    << " " << expr2 << " =\n"
                    << b << ".";
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult test_m3_near(const char* expr1, const char* expr2, const char* bounds_expr,
                                        const m3 &a, const m3 &b, const f_t bounds)
{
    for(int i = 0; i < 3; ++i)
        for(int j = 0; j < 3; ++j)
            if(fabs(a(i,j) - b(i,j)) > bounds)
                return ::testing::AssertionFailure()
                << "The difference between\n"
                <<  " " << a(i,j) << " = " << expr1 << "(" << i << "," << j << ")\n"
                <<  " " << b(i,j) << " = " << expr2 << "(" << i << "," << j << ")\n"
                << "is " << fabs(a(i,j) - b(i,j)) << "\n"
                << "where\n"
                << " " << expr1 << " =\n"
                << a << "\n"
                << " " << expr2 << " =\n"
                << b << ".";
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult test_v4_near(const char* expr1, const char* expr2, const char* bounds_expr,
                                        const v4 &a, const v4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i)
        if(fabs(a[i] - b[i]) > bounds)
            return ::testing::AssertionFailure()
                << "The difference between\n"
                <<  " " << a[i] << " = " << expr1 << "[" << i << "]\n"
                <<  " " << b[i] << " = " << expr2 << "[" << i << "]\n"
                << "is " << fabs(a[i]- b[i]) << "\n"
                << "where\n"
                << " " << expr1 << " =\n"
                << a << "\n"
                << " " << expr2 << " =\n"
                << b << ".";
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult test_v3_near(const char* expr1, const char* expr2, const char* bounds_expr,
                                        const v3 &a, const v3 &b, const f_t bounds)
{
    for(int i = 0; i < 3; ++i)
        if(fabs(a[i] - b[i]) > bounds)
            return ::testing::AssertionFailure()
            << "The difference between\n"
            <<  " " << a[i] << " = " << expr1 << "[" << i << "]\n"
            <<  " " << b[i] << " = " << expr2 << "[" << i << "]\n"
            << "is " << fabs(a[i]- b[i]) << "\n"
            << "where\n"
            << " " << expr1 << " =\n"
            << a << "\n"
            << " " << expr2 << " =\n"
            << b << ".";
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult test_quaternion_near(const char* expr1, const char* expr2, const char* bounds_expr,
                                                const quaternion &a, const quaternion &b, const f_t bounds)
{
    const char *n = nullptr; f_t a_n, b_n;
    if (fabs((a_n=a.w()) - (b_n=b.w())) > bounds) n = "w"; else
    if (fabs((a_n=a.x()) - (b_n=b.x())) > bounds) n = "x"; else
    if (fabs((a_n=a.y()) - (b_n=b.y())) > bounds) n = "y"; else
    if (fabs((a_n=a.z()) - (b_n=b.z())) > bounds) n = "z"; else
    return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure()
        << "The difference between\n"
        <<  " " << a_n << " = " << expr1 << "." << n << "\n"
        <<  " " << b_n << " = " << expr2 << "." << n << "\n"
        << "is " << fabs(a_n - b_n) << "\n"
        << "where\n"
        << " " << expr1 << " =\n"
        << a << "\n"
        << " " << expr2 << " =\n"
        << b << ".";
}

::testing::AssertionResult test_rotation_vector_near(const char* expr1, const char* expr2, const char* bounds_expr,
                                                     const rotation_vector &a, const rotation_vector &b, const f_t bounds)
{
    v4 A(a.x(), a.y(), a.z(), 0), B(b.x(), b.y(), b.z(), 0);
    if (A == v4::Zero() || B == v4::Zero()) {
        EXPECT_NEAR(fmod(A.norm(), 2*M_PI), 0, bounds)                 << "In " << expr1 << " " << expr2 << " where a = " << a;
        EXPECT_NEAR(fmod(B.norm(), 2*M_PI), 0, bounds)                 << "In " << expr1 << " " << expr2 << " where b = " << b;
    } else
        EXPECT_NEAR(fabs(A.normalized().dot(B.normalized())), 1, bounds) << "In " << expr1 << " " << expr2 << " where a = " << a << " b = " << b;
    EXPECT_NEAR(fmod((A-B).norm(), 2*M_PI), 0, bounds)                   << "In " << expr1 << " " << expr2 << " where a = " << a << " b = " << b;
    return ::testing::AssertionSuccess();
}

