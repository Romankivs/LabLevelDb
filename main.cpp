#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <iostream>
#include <string>

using json = nlohmann::json;

void create_product(leveldb::DB* db) {
    std::string name, category;
    double price;
    int stock;

    fmt::print("Enter product name: ");
    std::cin >> name;
    fmt::print("Enter product category: ");
    std::cin >> category;
    fmt::print("Enter product price: ");
    std::cin >> price;
    fmt::print("Enter product stock: ");
    std::cin >> stock;

    json product_json = {
        {"name", name},
        {"category", category},
        {"price", price},
        {"stock", stock}
    };

    std::string key = "Product:" + name + ":" + category;
    db->Put(leveldb::WriteOptions(), key, product_json.dump());
    fmt::print("Product '{}' added.\n", name);
}

void create_customer(leveldb::DB* db) {
    std::string name, email;

    fmt::print("Enter customer name: ");
    std::cin >> name;
    fmt::print("Enter customer email: ");
    std::cin >> email;

    json customer_json = {
        {"name", name},
        {"email", email},
        {"total_spent", 0.0},
        {"orders", json::array()}
    };

    std::string key = "Customer:" + email;
    db->Put(leveldb::WriteOptions(), key, customer_json.dump());
    fmt::print("Customer '{}' added.\n", name);
}

void create_order(leveldb::DB* db) {
    std::string order_code, customer_email;
    fmt::print("Enter order code: ");
    std::cin >> order_code;
    fmt::print("Enter customer email: ");
    std::cin >> customer_email;

    // Check if customer exists
    std::string customer_temp;
    if (!db->Get(leveldb::ReadOptions(), "Customer:" + customer_email, &customer_temp).ok()) {
        fmt::print("Error: Customer '{}' not found. Order creation aborted.\n", customer_email);
        return;
    }

    json products_json = json::array();

    while (true) {
        std::string add_product;
        fmt::print("Do you want to add a product? (yes/no): ");
        std::cin >> add_product;
        if (add_product != "yes") break;

        std::string name, category;
        int quantity;

        fmt::print("Enter product name: ");
        std::cin >> name;
        fmt::print("Enter product category: ");
        std::cin >> category;
        fmt::print("Enter quantity: ");
        std::cin >> quantity;

        products_json.push_back({
            {"name", name},
            {"category", category},
            {"quantity", quantity}
        });
    }

    json order_json = {
        {"order_code", order_code},
        {"customer_email", customer_email},
        {"products", products_json}
    };

    std::string order_key = "Order:" + order_code;
    double total_price = 0.0;

    // Create a WriteBatch for atomic operation
    leveldb::WriteBatch batch;

    for (auto& product : order_json["products"]) {
        std::string product_name = product["name"].get<std::string>();
        std::string category = product["category"].get<std::string>();
        int quantity = product["quantity"].get<int>();

        std::string product_key = "Product:" + product_name + ":" + category;
        std::string product_str;
        if (!db->Get(leveldb::ReadOptions(), product_key, &product_str).ok()) {
            fmt::print("Error: Product '{}' in category '{}' not found. Order creation aborted.\n", product_name, category);
            return;
        }

        // Check if there is enough stock
        json product_data = json::parse(product_str);
        int current_stock = product_data["stock"].get<int>();
        if (current_stock < quantity) {
            fmt::print("Error: Insufficient stock for '{}'. Available: {}, Requested: {}. Order creation aborted.\n", product_name, current_stock, quantity);
            return;
        }

        product_data["stock"] = current_stock - quantity;
        batch.Put(product_key, product_data.dump());

        total_price += product_data["price"].get<double>() * quantity;
    }

    // Add the total price to the order JSON and save the order in the WriteBatch
    order_json["total"] = total_price;
    batch.Put(order_key, order_json.dump());

    // Update the customer record to include the order and total spent
    std::string customer_key = "Customer:" + customer_email;
    std::string customer_data;
    if (db->Get(leveldb::ReadOptions(), customer_key, &customer_data).ok()) {
        json customer_json = json::parse(customer_data);
        customer_json["orders"].push_back(order_code); // Add order code to customer's orders
        customer_json["total_spent"] = customer_json["total_spent"].get<double>() + total_price; // Update total spent
        batch.Put(customer_key, customer_json.dump()); // Update the customer record in the batch
    }

    // Commit the batch to the database
    leveldb::WriteOptions write_options;
    leveldb::Status status = db->Write(write_options, &batch);
    if (!status.ok()) {
        fmt::print("Error creating order '{}': {}\n", order_code, status.ToString());
        return;
    }

    fmt::print("Order '{}' added with total price: {:.2f}. Total spent by customer: {:.2f}.\n", order_code, total_price, total_price);
}

void delete_product(leveldb::DB* db) {
    std::string name, category;
    fmt::print("Enter product name: ");
    std::cin >> name;
    fmt::print("Enter product category: ");
    std::cin >> category;

    std::string key = "Product:" + name + ":" + category;
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if (status.ok()) {
        fmt::print("Successfully deleted product '{}'.\n", name);
    } else {
        fmt::print("Error deleting product '{}': {}\n", name, status.ToString());
    }
}

void select_product(leveldb::DB* db) {
    std::string name, category;
    fmt::print("Enter product name: ");
    std::cin >> name;
    fmt::print("Enter product category: ");
    std::cin >> category;

    std::string key = "Product:" + name + ":" + category;
    std::string data;

    if (db->Get(leveldb::ReadOptions(), key, &data).ok()) {
        fmt::print("Product '{}':\n{}\n", name, data);
    } else {
        fmt::print("Error retrieving product '{}'. It may not exist.\n", name);
    }
}

void delete_customer(leveldb::DB* db) {
    std::string email;
    fmt::print("Enter customer email: ");
    std::cin >> email;

    std::string key = "Customer:" + email;
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if (status.ok()) {
        fmt::print("Successfully deleted customer '{}'.\n", email);
    } else {
        fmt::print("Error deleting customer '{}': {}\n", email, status.ToString());
    }
}

void select_customer(leveldb::DB* db) {
    std::string email;
    fmt::print("Enter customer email: ");
    std::cin >> email;

    std::string key = "Customer:" + email;
    std::string data;

    if (db->Get(leveldb::ReadOptions(), key, &data).ok()) {
        fmt::print("Customer '{}':\n{}\n", email, data);
    } else {
        fmt::print("Error retrieving customer '{}'. It may not exist.\n", email);
    }
}

void delete_order(leveldb::DB* db) {
    std::string order_code;
    fmt::print("Enter order code: ");
    std::cin >> order_code;

    std::string key = "Order:" + order_code;
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if (status.ok()) {
        fmt::print("Successfully deleted order '{}'.\n", order_code);
    } else {
        fmt::print("Error deleting order '{}': {}\n", order_code, status.ToString());
    }
}

void select_order(leveldb::DB* db) {
    std::string order_code;
    fmt::print("Enter order code: ");
    std::cin >> order_code;

    std::string key = "Order:" + order_code;
    std::string data;

    if (db->Get(leveldb::ReadOptions(), key, &data).ok()) {
        fmt::print("Order '{}':\n{}\n", order_code, data);
    } else {
        fmt::print("Error retrieving order '{}'. It may not exist.\n", order_code);
    }
}

void list_entities(leveldb::DB* db, const std::string& type) {
    json entities = json::array();
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());

    std::unique_ptr<leveldb::Iterator> iterator(it);

    for (iterator->Seek(type + ":"); iterator->Valid() && iterator->key().ToString().rfind(type + ":", 0) == 0; iterator->Next()) {
        json entity = json::parse(iterator->value().ToString());
        entities.push_back(entity);
    }

    if (entities.empty()) {
        fmt::print("No {} records found.\n", type);
    } else {
        fmt::print("{} List (JSON):\n{}\n", type, entities.dump(4)); // Call dump on the JSON array
    }
}

void cli(leveldb::DB* db) {
    std::string command;
    while (true) {
        fmt::print("\nAvailable commands:\n"
                        "  Create          |  Delete           |  Select           |  List\n"
                        "------------------|-------------------|-------------------|------------------\n"
                        "  create_product  |  delete_product   |  select_product   |  list_products\n"
                        "  create_customer |  delete_customer  |  select_customer  |  list_customers\n"
                        "  create_order    |  delete_order     |  select_order     |  list_orders\n"
                        "                  |                   |                   |  exit\n");

        fmt::print("Choose an action: ");
        std::cin >> command;

        if (command == "create_product") {
            create_product(db);
        } else if (command == "create_customer") {
            create_customer(db);
        } else if (command == "create_order") {
            create_order(db);
        } else if (command == "delete_product") {
            delete_product(db);
        } else if (command == "select_product") {
            select_product(db);
        } else if (command == "delete_customer") {
            delete_customer(db);
        } else if (command == "select_customer") {
            select_customer(db);
        } else if (command == "delete_order") {
            delete_order(db);
        } else if (command == "select_order") {
            select_order(db);
        } else if (command == "list_products") {
            list_entities(db, "Product");
        } else if (command == "list_customers") {
            list_entities(db, "Customer");
        } else if (command == "list_orders") {
            list_entities(db, "Order");
        } else if (command == "exit") {
            fmt::print("Exiting...\n");
            break;
        } else {
            fmt::print("Invalid command. Please try again.\n");
        }
    }
}

int main() {
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status = leveldb::DB::Open(options, "petshopdb", &db);
    if (!status.ok()) {
        fmt::print("Unable to open/create database: {}\n", status.ToString());
        return -1;
    }

    cli(db);
    delete db;
    return 0;
}