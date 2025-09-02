package bank;

import security.key.BankPublicKey;
import security.key.BankSymmetricKey;
import security.*;

public class MobileApp {

    private final String AppId;
    private String id, password;
    private BankSymmetricKey symKey;

    public MobileApp(String id, String password){
        this.id = id;
        this.password = password;
        AppId = randomUniqueStringGen();
    }

    private String randomUniqueStringGen() {
        return Encryptor.randomUniqueStringGen();
    }

    public String getAppId() {
        return AppId;
    }

    public Encrypted<BankSymmetricKey> sendSymKey(BankPublicKey publickey){
        symKey = new BankSymmetricKey(randomUniqueStringGen());
        return new Encrypted<BankSymmetricKey>(symKey, publickey);
    }

    public Encrypted<Message> deposit(int amount){
        Message msg = new Message("deposit", id, password, amount);
        return new Encrypted<Message>(msg, symKey);
    }

    public Encrypted<Message> withdraw(int amount){
        Message msg = new Message("withdraw", id, password, amount);
        return new Encrypted<Message>(msg, symKey);
    }

    public boolean processResponse(Encrypted<Boolean> obj) {
        if (obj == null || obj.decrypt(symKey) == null) {
            return false;
        }
        return obj.decrypt(symKey);
    }
}

