module System.Threading;
import System;

pragma (assembly, "mscorlib")
{
    typedef void delegate() ThreadStart;
    typedef void delegate(__object) ParameterizedThreadStart;

    public static final class Thread
    {
    public:
        this(shared ThreadStart);
        this(shared ParameterizedThreadStart);
        void Start();
        void Start(__object);
        void Join();
    }
}
