using System;

namespace Calculator
{
    public class Calculator
    {
        /// <summary>
        /// Method called by the CoreHook host to initialize the hooking library.
        /// </summary>
        /// <param name="remoteParameters">A pointer containing information about the hooking library.</param>
        public static void Load(IntPtr remoteParameters) => System.Diagnostics.Debug.WriteLine($"The pointer parameter was {remoteParameters.ToInt64():X16}.");
        /// <summary>
        /// Add <paramref name="a"/> to <paramref name="b"/>.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns><paramref name="a"/> plus <paramref name="b"/>.</returns>
        public static int Add(int a, int b) => a + b;
        /// <summary>
        /// Subtract <paramref name="a"/> from <paramref name="b"/>.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns><paramref name="b"/> minus <paramref name="a"/>.</returns>
        public static int Subtract(int a, int b) => b - a;
        /// <summary>
        /// Mulitply <paramref name="a"/> by <paramref name="b"/>.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns><paramref name="a"/> times <paramref name="b"/>.</returns>
        public static int Multiply(int a, int b) => a * b;
        /// <summary>
        /// Divide <paramref name="a"/> by <paramref name="b"/>.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns><paramref name="a"/> divied by <paramref name="b"/>.</returns>
        public static int Divide(int a, int b) => a / b;
    }
}
