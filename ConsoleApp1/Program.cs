using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


namespace ConsoleApp1
{
    class Program
    {
        static void Main1(string[] args)
        {
        int lastId  = 5000;
        int iRow, iCol,lastRow, lastCol;
        byte[,] bArr = new byte[lastId, lastId];
        double vSum=0;
        DateTime start1, start2, finish1, finish2;

            lastRow = bArr.GetUpperBound(0);
            lastCol = bArr.GetUpperBound(1);

            start1 = DateTime.Now;
        for (iRow = 0; iRow <= lastRow; iRow ++) 
            {
                for(iCol = 0; iCol<= lastCol; iCol++)
                {
                    vSum += bArr[iRow, iCol];
                }
        }
        finish1 = DateTime.Now;

        vSum = 0;
            start2 = DateTime.Now;
        foreach (byte bItem in bArr)
            {
                vSum += bItem;
            }

        finish2 = DateTime.Now;
        Console.WriteLine("for in for {0}", finish1 - start1);
        Console.WriteLine("for each {0}", finish2 - start2);
        Console.ReadKey();
        }

        static void Main(string[] args)
        {
            int lastId = 5000;
            int iRow, iCol, lastRow, lastCol;
            byte[,] bArr = new byte[lastId, lastId];
            double vSum = 0;
            DateTime start1, start2, finish1, finish2;

            lastRow = bArr.GetUpperBound(0);
            lastCol = bArr.GetUpperBound(1);

            unsafe
            {
                //fixed (byte* arr2 = bArr)
                {
                    start1 = DateTime.Now;
                    for (iRow = 0; iRow <= lastRow; iRow++)
                    {
                        for (iCol = 0; iCol <= lastCol; iCol++)
                        {
                            vSum += bArr[iRow, iCol];
                        }
                    }
                    finish1 = DateTime.Now;

                    vSum = 0;
                    start2 = DateTime.Now;
                    foreach (byte bItem in bArr)
                    {
                        vSum += bItem;
                    }

                    finish2 = DateTime.Now;
                    Console.WriteLine("for in for {0}", finish1 - start1);
                    Console.WriteLine("for each {0}", finish2 - start2);
                    Console.ReadKey();
                }
            }
        }
    }
}
