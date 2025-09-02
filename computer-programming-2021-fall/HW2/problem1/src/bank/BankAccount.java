package bank;

import bank.event.*;
import java.util.LinkedList;

class BankAccount {
    
    private LinkedList<Event> events = new LinkedList<>();
    final static int maxEvents = 100;
    private String id;
    private String password;
    private int balance;

    BankAccount(String id, String password, int balance) {
        this.id = id;
        this.password = password;
        this.balance = balance;
    }

    boolean authenticate(String password) {
        return this.password == password;
    }

    void deposit(int amount) {
        balance += amount;
        if (events.size() >= maxEvents) {
            events.removeFirst();
        }
        events.add(new DepositEvent());
    }

    boolean withdraw(int amount) {
        if (balance < amount) {
            return false;
        }
        balance -= amount;
        if (events.size() >= maxEvents) {
            events.removeFirst();
        }
        events.add(new WithdrawEvent());
        return true;
    }

    void receive(int amount) {
        balance += amount;
        if (events.size() >= maxEvents) {
            events.removeFirst();
        }
        events.add(new ReceiveEvent());
    }

    boolean send(int amount) {
        if (balance < amount) {
            return false;
        }
        balance -= amount;
        if (events.size() >= maxEvents) {
            events.removeFirst();
        }
        events.add(new SendEvent());
        return true;
    }

    Event[] getEvents() {
        Event[] eventsArray = new Event[events.size()];
        return events.toArray(eventsArray);
    }

    int getBalance() {
        return balance;
    }
}
