
square(i) = i*i
c = Channel(2)
isReady = Threads.Mutex();
Base.lock(isReady)
#put!(c, -1)
a = [0]

Threads.@threads for i = 1:Threads.nthreads()
    if Threads.threadid() == 2
        put!(c, 1)
        Base.unlock(isReady)
    elseif Threads.threadid() == 1
        Base.lock(isReady)
        a[1] = take!(c)
    end
end
