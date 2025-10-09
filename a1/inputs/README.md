# Assignment1

## Testing

You may use these input files and test cases to check your output with the expected results.

### Test cases for part 1:

For each test, your program should print the child process ID and the row number being searched, as specified in the assignment. Below are the only expected results for each input file.

- ./a1p1 < test1.txt  
Parent: The treasure was found by child with PID 155335 at row 36, column 154
- ./a1p1 < test2.txt   
Parent: The treasure was found by child with PID 155335 at row 91, column 0
- ./a1p1 < test3.txt  
Parent: No treasure was found in the matrix
- ./a1p1 < test4.txt  
Parent: The treasure was found by child with PID 155335 at row 0, column 365
- ./a1p1 < test5.txt  
Parent: The treasure was found by child with PID 155335 at row 78, column 255
- ./a1p1 < test6.txt  
Parent: The treasure was found by child with PID 155335 at row 58, column 999

### Test cases for part 2:

- ./a1p2 100 249 3  
    Child PID 2913699 checking range [100, 149]   
    Child PID 2913700 checking range [150, 199]   
    Child PID 2913701 checking range [200, 249]  

    Parent: All children finished. Primes found:    
    101 103 107 109 113 127 131 137 139 149 151 157 163 167 173 179 181 191 193 197 199 211 223 227 229 233 239 241

- ./a1p2 10 100 6  
    Child PID 3457361 checking range [10, 24]    
    Child PID 3457362 checking range [25, 39]    
    Child PID 3457363 checking range [40, 54]    
    Child PID 3457364 checking range [55, 69]    
    Child PID 3457365 checking range [70, 84]    
    Child PID 3457366 checking range [85, 100]    

    Parent: All children finished. Primes found:  
    11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 73 79 83 89 97

- ./a1p2 10000000 10000100 5    
    Child PID 2914849 checking range [10000000, 10000019]  
    Child PID 2914850 checking range [10000020, 10000039]  
    Child PID 2914851 checking range [10000040, 10000059]  
    Child PID 2914852 checking range [10000060, 10000079] 
    Child PID 2914853 checking range [10000080, 10000100]

    Parent: All children finished. Primes found:  
    10000019 10000079

- ./a1p2 99 100 10  
    Child PID 2912353 checking range [99, 99]   
    Child PID 2912354 checking range [100, 100] 

    Parent: All children finished. Primes found:

- ./a1p2 2000000000 2000000100 8  
    Child PID 2915670 checking range [2000000000, 2000000011]  
    Child PID 2915671 checking range [2000000012, 2000000023]  
    Child PID 2915672 checking range [2000000024, 2000000035]  
    Child PID 2915673 checking range [2000000036, 2000000047]  
    Child PID 2915674 checking range [2000000048, 2000000059]  
    Child PID 2915675 checking range [2000000060, 2000000071]  
    Child PID 2915676 checking range [2000000072, 2000000083]  
    Child PID 2915677 checking range [2000000084, 2000000100]  

    Parent: All children finished. Primes found:  
    2000000011 2000000033 2000000063 2000000087 2000000089 2000000099
