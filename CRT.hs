-- Chinese Remainder Solution based on
-- Euclid's algorithm, which you can never learn too many times, honest.
-- Calculates gcds of two numbers in an arbitrary euclidean domain.

module CRT (main, crt,crtm, liberty) where

-- Nicked verbatim from Standard Prelude.
euclid 0 0 =    error "Euclid.euclid: gcd(0,0) is undefined"
euclid x y =    euclid' (abs x) (abs y)
                where euclid' x 0 = x
                      euclid' x y = euclid' y (x `rem` y)

-- at least a little bit original...
exeuclid 0 0 =  error "Euclid.exeuclid: gcd(0,0) is undefined"
exeuclid x y =  exeuclid' (abs x) (abs y) where
                  exeuclid' x 0 = (x,1,0)
                  exeuclid' x y = (g,b,a-b*q) where
                    (q,r) = x `divMod` y
                    (g,a,b) = exeuclid' y r

crt [(a,m)] = a
crt ((a0,m0):(a1,m1):l) = crt (((a0*b1*m1+a1*b0*m0+mm) `mod` mm, mm):l)
                          where (1,b0,b1) = exeuclid m0 m1
                                mm = m0*m1

alpha  = 0.850639925845833
beta   = 0.96553505933836965
lambda = 2.97573419204335725
liberty m n = alpha * beta**(m+n) * lambda**(m*n)

moduli = map (2^64-) [0,3,5,7,9,11,15,17,33,35,39,45,47,53,57,59,63,75,77,83,87,89,95,99,105,113,117,119,125,129,143,147,153,155,165,173,179,183,189,195,197,209,215,243,249,255]

crtm as = crt $ zip as moduli

main = do inp <- getContents
          let ls = lines inp
          let as = map ((\[m,a] -> (a,m)) . map read . words) ls
          putStrLn $ show $ crt as
          putStrLn $ show $ product $ map snd as
