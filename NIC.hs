module NIC (main, moduli) where

import Data.List
import System.Environment

moduli = map (2^64-) [0,3,5,7,9,11,15,17,33,35,39,45,47,53,57,59,63,75,77,83,87,89,95,99,105,113,117,119,125,129,143,147,153,155,165,173,179,183,189,195,197,209,215,243,249,255]

eqlyx (_,_,_,a) (_,_,_,b) = a==b

addyx m (i0,n0,l0,yx) (i1,n1,l1,_) = ((i0+i1) `mod` m,
                                      (n0+n1) `mod` m,
                                      (l0+l1) `mod` m, yx)

main = do [arg] <- getArgs
          let modulus = moduli !! read arg :: Integer
          print modulus
          inp <- getContents
          let cnts = [(read newill, read needy, read legal, read yx) |
                      line <- lines inp,
                      "newillegal" `isPrefixOf` line,
                      let (_:newill:_:needy:_:legal:_:yx:_) = words line] ::
                      [(Integer,Integer,Integer,(Int,Int))]
          let cumcnts = map (foldr1 (addyx modulus)) $ groupBy eqlyx cnts
          let cnt1 = map (\(i,n,l,yx) ->  i+n+l  `mod` modulus) cumcnts
          let cnt3 = map (\(i,n,l,yx) -> 3*(n+l) `mod` modulus) cumcnts
          let diffs = zipWith (\x y-> (x-y) `mod` modulus) cnt1 (3:cnt3)
          let check = zipWith (\(i,n,l,yx) d -> (i,n,l,yx,d)) cumcnts diffs
          mapM_ print check
