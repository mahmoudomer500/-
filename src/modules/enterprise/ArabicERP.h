// ArabicERP.h - نظام إدارة موارد المؤسسات باللغة العربية
#ifndef ARABIC_ERP_H
#define ARABIC_ERP_H

#include <string>
#include <vector>
#include <map>

namespace ArabicERP {

    // هيكل المنتج في المخزن
    struct Product {
        int id;
        std::string name;
        std::string sku;
        double price;
        int quantity;
        std::string category;
    };

    // هيكل فاتورة المبيعات
    struct Invoice {
        int id;
        std::string customerName;
        std::vector<std::pair<int, int>> items; // productId, quantity
        double totalAmount;
        std::string date;
        bool isPaid;
    };

    class ERPManager {
    public:
        // إدارة المخزون
        static bool addProduct(const std::string& name, const std::string& sku, double price, int quantity);
        static bool updateStock(int productId, int change);
        static Product* getProduct(int id);
        static std::vector<Product> getLowStockItems(int threshold);

        // إدارة المبيعات
        static int createInvoice(const std::string& customerName, const std::vector<std::pair<int, int>>& items);
        static bool payInvoice(int invoiceId);
        static std::vector<Invoice> getInvoicesByCustomer(const std::string& customer);

        // التقارير المالية
        static double getTotalRevenue();
        static std::map<std::string, double> getSalesByCategory();

    private:
        static std::map<int, Product> inventory;
        static std::map<int, Invoice> invoices;
        static int nextProductId;
        static int nextInvoiceId;
    };

} // namespace ArabicERP

#endif // ARABIC_ERP_H
