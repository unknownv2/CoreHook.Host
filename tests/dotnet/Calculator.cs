using System;

namespace Calculator
{
    public class Calculator
    {
        /// <summary>
        /// Called by the CoreHook native host module to initialize the plugin with user-defined arguments.
        /// </summary>
        /// <param name="remoteParameters">A pointer containing information about the plugin to initialize.</param>
        public static void Load(IntPtr remoteParameters) => System.Diagnostics.Debug.WriteLine($"The pointer parameter was {remoteParameters.ToInt64():X16}.");
        /// <summary>
        /// Add <paramref name="a"/> to <paramref name="b"/>.
        /// </summary>
        /// <param name="a">An integer value</param>
        /// <param name="b">An integer value</param>
        /// <returns>The sum of <paramref name="a"/> and <paramref name="b"/>.</returns>
        public static int Add(int a, int b) => a + b;
        /// <summary>
        /// Subtract <paramref name="a"/> from <paramref name="b"/>.
        /// </summary>
        /// <param name="a">An integer value</param>
        /// <param name="b">An integer value</param>
        /// <returns>The difference of <paramref name="b"/> and <paramref name="a"/>.</returns>
        public static int Subtract(int a, int b) => b - a;
        /// <summary>
        /// Multiply <paramref name="a"/> by <paramref name="b"/>.
        /// </summary>
        /// <param name="a">The multiplicand</param>
        /// <param name="b">The multiplier</param>
        /// <returns><paramref name="a"/> times <paramref name="b"/>.</returns>
        public static int Multiply(int a, int b) => a * b;
        /// <summary>
        /// Divide <paramref name="a"/> by <paramref name="b"/>.
        /// </summary>
        /// <param name="a">The dividend</param>
        /// <param name="b">The divisor</param>
        /// <returns>The quotient of <paramref name="a"/> and <paramref name="b"/>.</returns>
        public static int Divide(int a, int b) => a / b;
    }
}
