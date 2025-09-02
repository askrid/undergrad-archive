package hand.market;

import hand.agent.Buyer;
import hand.agent.Seller;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Random;

class Pair<K,V> {
    public K key;
    public V value;
    Pair(K key, V value) {
        this.key = key;
        this.value = value;
    }
}

public class Market {
    public ArrayList<Buyer> buyers;
    public ArrayList<Seller> sellers;

    public Market(int nb, ArrayList<Double> fb, int ns, ArrayList<Double> fs) {
        buyers = createBuyers(nb, fb);
        sellers = createSellers(ns, fs);
    }
    
    private ArrayList<Buyer> createBuyers(int n, ArrayList<Double> f) {
        ArrayList<Buyer> buyers = new ArrayList<>();
        for (int i = 1; i <= n; i++) {
            buyers.add(new Buyer(calculatePoly(f, (double) i / (double) n)));
        }
        return buyers;
    }

    private ArrayList<Seller> createSellers(int n, ArrayList<Double> f) {
        ArrayList<Seller> sellers = new ArrayList<>();
        for (int i = 1; i <= n; i++) {
            sellers.add(new Seller(calculatePoly(f, (double) i / (double) n)));
        }
        return sellers;
    }

    private double calculatePoly(ArrayList<Double> f, double x) {
        double res = 0;
        for (int i = 0; i < f.size(); i++) {
            res += f.get(i) * Math.pow(x, i);
        }
        return res;
    }

    private ArrayList<Pair<Seller, Buyer>> matchedPairs(int day, int round) {
        ArrayList<Seller> shuffledSellers = new ArrayList<>(sellers);
        ArrayList<Buyer> shuffledBuyers = new ArrayList<>(buyers);
        Collections.shuffle(shuffledSellers, new Random(71 * day + 43 * round + 7));
        Collections.shuffle(shuffledBuyers, new Random(67 * day + 29 * round + 11));
        ArrayList<Pair<Seller, Buyer>> pairs = new ArrayList<>();
        for (int i = 0; i < shuffledBuyers.size(); i++) {
            if (i < shuffledSellers.size()) {
                pairs.add(new Pair<>(shuffledSellers.get(i), shuffledBuyers.get(i)));
            }
        }
        return pairs;
    }

    public double simulate() {
        double avgPrice = 0;
        int numTransactions = 0;
        for (int day = 1; day <= 3000; day++) { // do not change this line
            for (int round = 1; round <= 5; round++) { // do not change this line
                ArrayList<Pair<Seller, Buyer>> pairs = matchedPairs(day, round); // do not change this line
                for (Pair<Seller, Buyer> pair : pairs) {
                    Seller seller = pair.key;
                    Buyer buyer = pair.value;
                    double suggestedPrice = seller.getExpectedPrice();
                    if (buyer.willTransact(suggestedPrice) && seller.willTransact(suggestedPrice)) {
                        seller.makeTransaction();
                        buyer.makeTransaction();
                    }
                }
            }
            if (day == 3000) {
                for (Seller seller : sellers) {
                    if (seller.getHadTransaction()) {
                        avgPrice += seller.getExpectedPrice();
                        numTransactions++;
                    }
                }
                avgPrice /= (double) numTransactions;
            }
            reflectAll();
        }
        return avgPrice;
    }

    private void reflectAll() {
        for (Buyer buyer : buyers) {
            buyer.reflect();
        }
        for (Seller seller : sellers) {
            seller.reflect();
        }
    }
}

