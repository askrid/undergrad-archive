package bank;

public class Session {
    
    private String sessionKey;
    private Bank bank;
    private boolean valid;
    private int transLimit = 3;

    Session(String sessionKey, Bank bank){
        this.sessionKey = sessionKey;
        this.bank = bank;
        valid = true;
    }

    public boolean deposit(int amount) {
        if (!valid) {
            return false;
        }
        boolean res = bank.deposit(sessionKey, amount);
        if (--transLimit <= 0) {
            SessionManager.expireSession(this);
        }
        return res;
    }

    public boolean withdraw(int amount) {
        if (!valid) {
            return false;
        }
        boolean res = bank.withdraw(sessionKey, amount);
        if (--transLimit <= 0) {
            SessionManager.expireSession(this);
        }
        return res;
    }

    public boolean transfer(String targetId, int amount) {
        if (!valid) {
            return false;
        }
        boolean res = bank.transfer(sessionKey, targetId, amount);
        if (--transLimit <= 0) {
            SessionManager.expireSession(this);
        }
        return res;
    }

    void setValid(boolean valid) {
        this.valid = valid;
    }

    String getSessionKey() {
        return sessionKey;
    }

    Bank getBank() {
        return bank;
    }
}
