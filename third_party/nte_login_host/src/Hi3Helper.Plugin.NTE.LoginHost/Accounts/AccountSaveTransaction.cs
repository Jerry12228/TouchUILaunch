namespace Nte.LoginHost.Accounts;

// A newly created account must not survive a rejected admission or failed
// callback. Existing records are never removed by this transaction.
internal static class AccountSaveTransaction
{
    internal static async Task<LocalAccount> SaveAndDeliverAsync(Func<Task<PendingAccountSave>> prepare, Func<Task> deliver)
    {
        PendingAccountSave change = await prepare();
        try { await deliver(); }
        catch (Exception failure)
        {
            try { change.Dispose(); }
            catch (Exception rollbackFailure) { throw new AggregateException("Failed to restore saved account after login failed.", failure, rollbackFailure); }
            throw;
        }
        change.Commit(); return change.Account;
    }
    internal static async Task<LocalAccount> SaveAndDeliverAsync(Func<Task<LocalAccount>> save,
        Func<Task> deliver, Action<Guid> rollback)
    {
        LocalAccount created = await save();
        try { await deliver(); return created; }
        catch (Exception failure)
        {
            try { rollback(created.Id); }
            catch (Exception rollbackFailure) { throw new AggregateException("Failed to undo the new local account after login failed.", failure, rollbackFailure); }
            throw;
        }
    }
}
