// ArabicERP.cpp - تطبيق نظام إدارة موارد المؤسسات
#include "ArabicERP.h"
#include <iostream>
#include <iomanip>

namespace ArabicERP {

    std::map<int, Product> ERPManager::inventory;
    std::map<int, Invoice> ERPManager::invoices;
    int ERPManager::nextProductId = 1001;
    int ERPManager::nextInvoiceId = 5001;

    bool ERPManager::addProduct(const std::string& name, const std::string& sku, double price, int quantity) {
        Product p = { nextProductId++, name, sku, price, quantity, "عام" };
        inventory[p.id] = p;
        std::cout << "📦 تم إضافة منتج جديد للمخزن: " << name << " (SKU: " << sku << ")" << std::endl;
        return true;
    }

    bool ERPManager::updateStock(int productId, int change) {
        if (inventory.count(productId)) {
            inventory[productId].quantity += change;
            std::cout << "🔄 تحديث المخزن للمنتج " << inventory[productId].name << ": " << inventory[productId].quantity << std::endl;
            return true;
        }
        return false;
    }

    Product* ERPManager::getProduct(int id) {
        if (inventory.count(id)) return &inventory[id];
        return nullptr;
    }

    std::vector<Product> ERPManager::getLowStockItems(int threshold) {
        std::vector<Product> lowStock;
        for (auto const& [id, p] : inventory) {
            if (p.quantity <= threshold) {
                lowStock.push_back(p);
            }
        }
        return lowStock;
    }

    int ERPManager::createInvoice(const std::string& customerName, const std::vector<std::pair<int, int>>& items) {
        double total = 0;
        for (auto const& item : items) {
            Product* p = getProduct(item.first);
            if (p && p->quantity >= item.second) {
                total += p->price * item.second;
                updateStock(p->id, -item.second);
            } else {
                std::cout << "❌ فشل إنشاء الفاتورة: كمية غير كافية من المنتج " << (p ? p->name : "غير معروف") << std::endl;
                return -1;
            }
        }

        Invoice inv = { nextInvoiceId++, customerName, items, total, "2026-08-24", false };
        invoices[inv.id] = inv;
        std::cout << "🧾 تم إنشاء فاتورة جديدة للعميل " << customerName << " بمبلغ: " << total << std::endl;
        return inv.id;
    }

    bool ERPManager::payInvoice(int invoiceId) {
        if (invoices.count(invoiceId)) {
            invoices[invoiceId].isPaid = true;
            std::cout << "✅ تم دفع الفاتورة رقم: " << invoiceId << std::endl;
            return true;
        }
        return false;
    }

    double ERPManager::getTotalRevenue() {
        double revenue = 0;
        for (auto const& [id, inv] : invoices) {
            if (inv.isPaid) revenue += inv.totalAmount;
        }
        return revenue;
    }

    std::map<std::string, double> ERPManager::getSalesByCategory() {
        std::map<std::string, double> sales;
        for (auto const& [id, inv] : invoices) {
            if (inv.isPaid) {
                for (auto const& item : inv.items) {
                    Product* p = getProduct(item.first);
                    if (p) sales[p->category] += p->price * item.second;
                }
            }
        }
        return sales;
    }

} // namespace ArabicERP
