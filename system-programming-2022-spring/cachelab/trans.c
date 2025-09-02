/* Matrix transpose B = A^T, implemented by J. W. Choi */

#include <stdio.h>
#include "cachelab.h"

/* Hard coded special case for 64x64 matrix transpose. */
void tp_64(int M, int N, int A[N][M], int B[M][N]);

/* General case matrix transpose. */
void tp_gen(int M, int N, int A[N][M], int B[M][N]);

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 64 && N == 64)
        /* Special case for 64x64. */
        tp_64(M, N, A, B);
    else
        /* General cases. */
        tp_gen(M, N, A, B);
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    registerTransFunction(transpose_submit, transpose_submit_desc);
}

void tp_64(int M, int N, int A[N][M], int B[M][N])
{
    int brow, bcol;                     /* Block row and column with stride of 8. */
    int row, col;                       /* Row and column with stride of 1. */
    int t0, t1, t2, t3, t4, t5, t6, t7; /* Temporary variables. */

    if (M != 64 || N != 64)
        return;

    for (brow = 0; brow < N; brow += 8)
    {
        for (bcol = 0; bcol < M; bcol += 8)
        {
            /* We divide a block of A and B into 4 pieces:
             A00 A01    B00 B01
             A10 A11    B10 B11 */

            /* Transpose and copy A00 to B00, and A01 to B01 as well.
             It would be a waste if we only cache 4 integers of A on each iteration,
             since a line of cache can store up to 8 integers. So we temporarily store
             A01 in B01. Later we could copy B01 to B10, enjoying decent spacial locality. */
            for (row = brow; row < brow + 4; row++)
            {
                /* Store a row of A00 and A01 in t0~t7. */
                t0 = A[row][bcol + 0];
                t1 = A[row][bcol + 1];
                t2 = A[row][bcol + 2];
                t3 = A[row][bcol + 3];
                t4 = A[row][bcol + 4];
                t5 = A[row][bcol + 5];
                t6 = A[row][bcol + 6];
                t7 = A[row][bcol + 7];

                /* Copy a row of A00 to a column of B00. */
                B[bcol + 0][row] = t0;
                B[bcol + 1][row] = t1;
                B[bcol + 2][row] = t2;
                B[bcol + 3][row] = t3;

                /* Copy a row of A01 to a column of B01.
                 Notice that it uses the same rows from the previous statements,
                 enjoying good spacial locality. */
                B[bcol + 0][row + 4] = t4;
                B[bcol + 1][row + 4] = t5;
                B[bcol + 2][row + 4] = t6;
                B[bcol + 3][row + 4] = t7;
            }

            /* Transpose and copy A10 to B01, and copy B01 to B10.
             Remember that A01 has been transposed and stored in B01. */
            for (col = bcol; col < bcol + 4; col++)
            {
                /* Store a column of A10 in t0~t3. */
                t0 = A[brow + 4][col];
                t1 = A[brow + 5][col];
                t2 = A[brow + 6][col];
                t3 = A[brow + 7][col];

                /* Store a row of B01 in t4~t7. */
                t4 = B[col][brow + 4];
                t5 = B[col][brow + 5];
                t6 = B[col][brow + 6];
                t7 = B[col][brow + 7];

                /* Copy a column of A10 to a row of B01.
                 Notice that it uses the same locations from the prior statements. */
                B[col][brow + 4] = t0;
                B[col][brow + 5] = t1;
                B[col][brow + 6] = t2;
                B[col][brow + 7] = t3;

                /* Copy a row of B01 to a row of B10.
                 Since 'col + 4' has same set index as 'col', the rows of A won't evicted twice. */
                B[col + 4][brow + 0] = t4;
                B[col + 4][brow + 1] = t5;
                B[col + 4][brow + 2] = t6;
                B[col + 4][brow + 3] = t7;
            }

            /* Transpose and copy A11 to B11. */
            for (row = brow + 4; row < brow + 8; row++)
            {
                /* Store a row of A11 in t0~t3. */
                t0 = A[row][bcol + 4];
                t1 = A[row][bcol + 5];
                t2 = A[row][bcol + 6];
                t3 = A[row][bcol + 7];

                /* Copy a row of A11 to a column of B11. */
                B[bcol + 4][row] = t0;
                B[bcol + 5][row] = t1;
                B[bcol + 6][row] = t2;
                B[bcol + 7][row] = t3;
            }
        }
    }
}

void tp_gen(int M, int N, int A[N][M], int B[M][N])
{
    int bsize, brow, bcol; /* Block row and column with stride of bsize. */
    int row, col;          /* Row and column with stride of 1. */
    int dval;              /* Stores diagonal value of A. */

    /* Optimal size for block is 8 in many cases.
     But use 16 if matrix size is not divided by 8. */
    if (M % 8 || N % 8)
        bsize = 16;
    else
        bsize = 8;

    for (brow = 0; brow < N; brow += bsize)
    {
        for (bcol = 0; bcol < M; bcol += bsize)
        {
            /* Each second term of below loop conditions is necessary if the matrix size is not divided by 8. */
            for (row = brow; (row < brow + bsize) && (row < N); row++)
            {
                for (col = bcol; (col < bcol + bsize) && (col < M); col++)
                {
                    /* Avoid cache eviction on the diagonal of the matrix.
                     This helps a row of A keep being cached. */
                    if (row == col)
                    {
                        dval = A[row][col];
                    }
                    else
                    {
                        B[col][row] = A[row][col];
                    }
                }

                /* Diagonal values exist if and only if the block is also diagonal in the matrix. */
                if (brow == bcol)
                    B[row][row] = dval;
            }
        }
    }
}