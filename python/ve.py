import matplotlib.pyplot as plt
import networkx as nx

# Tạo graph
G = nx.DiGraph()

# ===== Actors =====
actors = {
    "Driver": (0, 4),
    "Admin": (0, 1),
    "Arduino": (0, 2.5)
}

# ===== Use Cases =====
use_cases = {
    "Xe vào bãi": (3, 4),
    "Xe ra bãi": (3, 3),
    "Phát hiện trạng thái slot": (6, 4.5),
    "Cập nhật số chỗ trống": (6, 3.8),
    "Ghi log xe vào": (6, 3),
    "Ghi log xe ra": (6, 2.3),
    "Gửi dữ liệu qua COM": (9, 3.5),
    "Lưu trạng thái bãi đỗ": (12, 3.5),
    "Xem trạng thái bãi đỗ": (3, 1.5),
    "Xem log xe ra vào": (3, 0.8),
    "Lọc log theo thời gian": (6, 0.8)
}

# Thêm node
for node, pos in {**actors, **use_cases}.items():
    G.add_node(node, pos=pos)

# ===== Quan hệ actor =====
edges_actor = [
    ("Driver", "Xe vào bãi"),
    ("Driver", "Xe ra bãi"),
    ("Admin", "Xem trạng thái bãi đỗ"),
    ("Admin", "Xem log xe ra vào"),
    ("Arduino", "Phát hiện trạng thái slot"),
    ("Arduino", "Cập nhật số chỗ trống"),
    ("Arduino", "Gửi dữ liệu qua COM")
]

# ===== Include =====
include_edges = [
    ("Xe vào bãi", "Phát hiện trạng thái slot"),
    ("Xe vào bãi", "Cập nhật số chỗ trống"),
    ("Xe vào bãi", "Ghi log xe vào"),
    ("Xe ra bãi", "Phát hiện trạng thái slot"),
    ("Xe ra bãi", "Cập nhật số chỗ trống"),
    ("Xe ra bãi", "Ghi log xe ra"),
    ("Cập nhật số chỗ trống", "Gửi dữ liệu qua COM"),
    ("Ghi log xe vào", "Gửi dữ liệu qua COM"),
    ("Ghi log xe ra", "Gửi dữ liệu qua COM"),
    ("Gửi dữ liệu qua COM", "Lưu trạng thái bãi đỗ")
]

# ===== Extend =====
extend_edges = [
    ("Lọc log theo thời gian", "Xem log xe ra vào")
]

# Thêm edge
G.add_edges_from(edges_actor)
G.add_edges_from(include_edges)
G.add_edges_from(extend_edges)

pos = nx.get_node_attributes(G, 'pos')

plt.figure(figsize=(16, 8))

# Vẽ actors
nx.draw_networkx_nodes(
    G, pos,
    nodelist=actors.keys(),
    node_shape='s',
    node_size=2500,
    node_color="#AED6F1"
)

# Vẽ use cases
nx.draw_networkx_nodes(
    G, pos,
    nodelist=use_cases.keys(),
    node_shape='o',
    node_size=3000,
    node_color="#D5F5E3"
)

# Vẽ quan hệ actor
nx.draw_networkx_edges(G, pos, edgelist=edges_actor, width=2)

# Vẽ include (nét đứt)
nx.draw_networkx_edges(
    G, pos,
    edgelist=include_edges,
    style="dashed",
    width=1.5
)

# Vẽ extend (chấm gạch)
nx.draw_networkx_edges(
    G, pos,
    edgelist=extend_edges,
    style="dotted",
    width=1.5
)

# Nhãn
nx.draw_networkx_labels(G, pos, font_size=9)

# Ghi chú include / extend
plt.text(5.2, 5.2, "<<include>>", fontsize=9)
plt.text(5.2, 0.2, "<<extend>>", fontsize=9)

plt.title("Use Case Diagram – Hệ thống quản lý bãi đỗ xe", fontsize=14)
plt.axis("off")
plt.tight_layout()
plt.show()
