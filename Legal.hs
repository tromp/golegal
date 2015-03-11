import Data.Array
import Data.List
import Data.Char
import Numeric

alpha  = 0.850639925845714538
logalpha = logBase 10 alpha

beta   = 0.96553505933837387
logbeta = logBase 10 beta

lambda = 2.97573419204335724938
loglambda = logBase 10 lambda

legal m n = alpha * beta**(m+n) * lambda**(m*n)
loglegal m n = logalpha + logbeta*(m+n) + loglambda*m*n


llegal m n = (a, 10.0**b) where (a,b) = properFraction (loglegal m n)

recurrence :: [Integer] -> [Integer] -> [Integer]
recurrence coeff basis = rec where
 rec = basis ++ rest
 rest = foldl (zipWith (+)) (repeat 0) $ zipWith (map.(*)) coeff $ tails rec

fib = recurrence [1,1] [0,1]

legal1 = recurrence [1,-1,3] [1,1,5]

legal2 = recurrence [-1,2,20,-13,31,-16,10] [1,5,57,489,4125,35117,299681]

legal3 = recurrence [-5,73,100,-1402,1014,-5352,-2490,6018,-4020,1766,9083,-19993,22072,-16646,9426,-3750,1171,-233,33] [1,15,489,12675,321689,8180343,208144601,5296282323,134764135265,3429075477543,87252874774409,2220150677358587,56491766630430761,1437433832683612783,36575525011037967769,930664771769562054147,23680778803620700205625,602557764897193682879119,15332091188757329557096929]

ternary n = showIntAtBase 3 intToDigit n ""
