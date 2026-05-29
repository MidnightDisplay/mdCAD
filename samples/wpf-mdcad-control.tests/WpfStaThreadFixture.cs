using System.Threading;
using System.Threading.Tasks;
using System.Windows.Threading;

namespace MdCad.Wpf.Control.Tests;

public sealed class WpfStaThreadFixture
{
    public Task RunAsync(Func<Task> action)
    {
        return RunAsync(async () =>
        {
            await action();
            return true;
        });
    }

    public Task<T> RunAsync<T>(Func<Task<T>> action)
    {
        TaskCompletionSource<T> completionSource = new(TaskCreationOptions.RunContinuationsAsynchronously);
        Thread thread = new(() =>
        {
            Dispatcher dispatcher = Dispatcher.CurrentDispatcher;
            SynchronizationContext.SetSynchronizationContext(new DispatcherSynchronizationContext(dispatcher));

            _ = dispatcher.InvokeAsync(async () =>
            {
                try
                {
                    T result = await action();
                    completionSource.SetResult(result);
                }
                catch (Exception ex)
                {
                    completionSource.SetException(ex);
                }
                finally
                {
                    dispatcher.BeginInvokeShutdown(DispatcherPriority.Background);
                }
            });

            Dispatcher.Run();
        });

        thread.SetApartmentState(ApartmentState.STA);
        thread.Start();
        return completionSource.Task;
    }
}
