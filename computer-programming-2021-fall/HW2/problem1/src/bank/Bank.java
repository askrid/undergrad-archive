package bank;

import bank.event.*;
import security.*;
import security.key.*;
import java.util.HashMap;

public class Bank {
    
    private HashMap<String, BankAccount> accounts = new HashMap<>();
    final static int maxAccounts = 100;

    public void createAccount(String id, String password) {
        createAccount(id, password, 0);
    }

    public void createAccount(String id, String password, int initBalance) {
        if (accounts.containsKey(id) || accounts.size() >= maxAccounts) {
            return;
        }
        accounts.put(id, new BankAccount(id, password, initBalance));
    }

    public boolean deposit(String id, String password, int amount) {
        BankAccount account = accounts.get(id);
        if (account == null || !account.authenticate(password)) {
            return false;
        }
        account.deposit(amount);
        return true;
    }

    public boolean withdraw(String id, String password, int amount) {
        BankAccount account = accounts.get(id);
        if (account == null || !account.authenticate(password)) {
            return false;
        }
        return account.withdraw(amount);
    }

    public boolean transfer(String sourceId, String password, String targetId, int amount) {
        BankAccount source = accounts.get(sourceId);
        BankAccount target = accounts.get(targetId);
        if (source == null || target == null || !source.authenticate(password)) {
            return false;
        }
        if (!source.send(amount)) {
            return false;
        }
        target.receive(amount);
        return true;
    }

    public Event[] getEvents(String id, String password) {
        BankAccount account = accounts.get(id);
        if (account == null || !account.authenticate(password)) {
            return null;
        }
        return account.getEvents();
    }

    public int getBalance(String id, String password) {
        BankAccount account = accounts.get(id);
        if (account == null || !account.authenticate(password)) {
            return -1;
        }
        return account.getBalance();
    }

    HashMap<String, BankAccount> sessions = new HashMap();
    final static int maxSessionKey = 100;

    private static String randomUniqueStringGen(){
        return Encryptor.randomUniqueStringGen();
    }

    String generateSessionKey(String id, String password){
        BankAccount account = accounts.get(id);
        if(account == null || !account.authenticate(password) || sessions.size() >= maxSessionKey){
            return null;
        }
        String sessionkey = randomUniqueStringGen();
        sessions.put(sessionkey, account);
        return sessionkey;
    }
    
    BankAccount getAccount(String sessionkey) {
        return sessions.get(sessionkey);
    }

    void removeSession(String sessionkey) {
        sessions.remove(sessionkey);
    }

    boolean deposit(String sessionkey, int amount) {
        BankAccount account = getAccount(sessionkey);
        if (account == null) {
            return false;
        }
        account.deposit(amount);
        return true;
    }

    boolean withdraw(String sessionkey, int amount) {
        BankAccount account = getAccount(sessionkey);
        if (account == null) {
            return false;
        }
        return account.withdraw(amount);
    }

    boolean transfer(String sessionkey, String targetId, int amount) {
        BankAccount source = getAccount(sessionkey);
        BankAccount target = accounts.get(targetId);
        if (source == null || target == null) {
            return false;
        }
        if (!source.send(amount)) {
            return false;
        }
        target.receive(amount);
        return true;
    }

    private BankSecretKey secretKey;
    final static int maxHandshakes = 10000;
    HashMap<String, BankSymmetricKey> bankSymmetricKeys = new HashMap<>();

    public BankSymmetricKey getSymmetricKey(String AppId) {
        return bankSymmetricKeys.get(AppId);
    }

    public BankPublicKey getPublicKey() {
        BankKeyPair keypair = Encryptor.publicKeyGen(); // generates two keys : BankPublicKey, BankSecretKey
        secretKey = keypair.deckey; // stores BankSecretKey internally
        return keypair.enckey;
    }

    public void fetchSymKey(Encrypted<BankSymmetricKey> encryptedKey, String AppId){
        if (encryptedKey == null || !bankSymmetricKeys.containsKey(AppId) && bankSymmetricKeys.size() >= maxHandshakes) {
            return;
        }
        BankSymmetricKey symKey = encryptedKey.decrypt(secretKey);
        if (symKey == null) {
            return;
        }
        bankSymmetricKeys.put(AppId, symKey);
    }

    public Encrypted<Boolean> processRequest(Encrypted<Message> messageEnc, String AppId) {
        BankSymmetricKey symKey = bankSymmetricKeys.get(AppId);
        if (symKey == null) {
            return null;
        }
        Message msg = messageEnc.decrypt(symKey);
        if (msg == null) {
            return null;
        }
        boolean res = false;
        if (msg.getRequestType() == "deposit") {
            res = deposit(msg.getId(), msg.getPassword(), msg.getAmount());
        } else if (msg.getRequestType() == "withdraw") {
            res = withdraw(msg.getId(), msg.getPassword(), msg.getAmount());
        }
        return new Encrypted<Boolean>(res, symKey);
    }
}