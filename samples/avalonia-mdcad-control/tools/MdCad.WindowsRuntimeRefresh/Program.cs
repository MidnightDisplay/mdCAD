using MdCad.Avalonia.Control.Tools.RuntimeRefresh;

RuntimeRefreshOptions options = RuntimeRefreshOptions.Parse(args);
RuntimeRefreshOrchestrator orchestrator = new();
await orchestrator.RunAsync(options, CancellationToken.None);
