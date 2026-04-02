return {
  entities = {
    { id = "geometry_10", type = "line", a = {0, 0, 0}, b = {4, 0, 0} },
    { id = "geometry_11", type = "line", a = {0, 0, 0}, b = {2, 2, 0} },
    { id = "geometry_12", type = "arc", center = {2, 1, 0}, radius = 1.0, start_angle = 0, end_angle = 1.5708, normal = {0, 0, 1} }
  },
  constraints = {
    { id = "constraint_10", type = "Length", participants = {"geometry_10"}, value = 4.0, driven = false },
    { id = "constraint_11", type = "Angle", participants = {"geometry_10", "geometry_11"}, value = 45.0, driven = true }
  }
}
