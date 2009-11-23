/*  This file is part of the Vc library.

    Copyright (C) 2009 Matthias Kretz <kretz@kde.org>

    Vc is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    Vc is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Vc.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <Vc/Vc>
#include "benchmark.h"
#include "random.h"
#include "cpuid.h"
#include <cstdio>
#include <cstdlib>

using namespace Vc;

template<typename T, int S> struct KeepResultsHelper { static inline void keep(const T &tmp0) { asm volatile(""::"r"(tmp0)); } };
#ifdef VC_IMPL_SSE
template<typename T> struct KeepResultsHelper<T, 16> { static inline void keep(const T &tmp0) { asm volatile(""::"x"(reinterpret_cast<const __m128 &>(tmp0))); } };
template<typename T> struct KeepResultsHelper<T, 32> { static inline void keep(const T &tmp0) {
    asm volatile(""::"x"(reinterpret_cast<const __m128 &>(tmp0)), "x"(reinterpret_cast<const __m128 *>(&tmp0)[1]));
} };
#endif
#ifdef VC_IMPL_LRBni
template<typename T> struct KeepResultsHelper<T, 64> { static inline void keep(const T &tmp0) {
    asm volatile(""::"x"(reinterpret_cast<const __m128 &>(tmp0)), "x"(reinterpret_cast<const __m128 *>(&tmp0)[1]), "x"(reinterpret_cast<const __m128 *>(&tmp0)[2]), "x"(reinterpret_cast<const __m128 *>(&tmp0)[3]));
} };
#endif

template<typename T> static inline void keepResults(const T &tmp0)
{
    KeepResultsHelper<T, sizeof(T)>::keep(tmp0);
}

template<typename Vector> class DoMemIos
{
    enum {
        Factor2 = 128
    };
    public:
        static void run(const int Repetitions)
        {
            const int Factor = CpuId::L1Data() / (sizeof(Vector) * 4); // quarter L1
            Vector *a = new Vector[Factor];

            {
                Benchmark timer("write", sizeof(Vector) * Factor * Factor2, "Byte");
                const Vector foo = PseudoRandom<Vector>::next();
                for (int repetitions = 0; repetitions < Repetitions; ++repetitions) {
                    timer.Start();
                    for (int j = 0; j < Factor2; ++j) {
                        keepResults(foo);
                        for (int i = 0; i < Factor; i += 4) {
                            a[i + 0] = foo;
                            a[i + 1] = foo;
                            a[i + 2] = foo;
                            a[i + 3] = foo;
                        }
                    }
                    timer.Stop();
                }
                timer.Print(Benchmark::PrintAverage);
            }
            {
                Benchmark timer("read", sizeof(Vector) * Factor * Factor2, "Byte");
                for (int repetitions = 0; repetitions < Repetitions; ++repetitions) {
                    timer.Start();
                    for (int j = 0; j < Factor2; ++j) {
                        for (int i = 0; i < Factor; i += 4) {
                            const Vector &tmp0 = a[i + 0]; keepResults(tmp0);
                            const Vector &tmp1 = a[i + 1]; keepResults(tmp1);
                            const Vector &tmp2 = a[i + 2]; keepResults(tmp2);
                            const Vector &tmp3 = a[i + 3]; keepResults(tmp3);
                        }
                    }
                    timer.Stop();
                }
                timer.Print(Benchmark::PrintAverage);
            }
            delete[] a;
        }
};

int bmain(Benchmark::OutputMode out)
{
    const int Repetitions = out == Benchmark::Stdout ? 10 : g_Repetitions > 0 ? g_Repetitions : 200;

    Benchmark::addColumn("datatype");

    Benchmark::setColumnData("datatype", "double_v");
    DoMemIos<double_v>::run(Repetitions);
    Benchmark::setColumnData("datatype", "float_v");
    DoMemIos<float_v>::run(Repetitions);
    Benchmark::setColumnData("datatype", "int_v");
    DoMemIos<int_v>::run(Repetitions);
    Benchmark::setColumnData("datatype", "uint_v");
    DoMemIos<uint_v>::run(Repetitions);
    Benchmark::setColumnData("datatype", "short_v");
    DoMemIos<short_v>::run(Repetitions);
    Benchmark::setColumnData("datatype", "ushort_v");
    DoMemIos<ushort_v>::run(Repetitions);
    Benchmark::setColumnData("datatype", "sfloat_v");
    DoMemIos<sfloat_v>::run(Repetitions);

    return 0;
}