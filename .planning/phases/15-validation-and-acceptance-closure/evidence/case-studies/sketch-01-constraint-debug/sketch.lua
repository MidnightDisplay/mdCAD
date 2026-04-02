return {
  entities = {
    { id = "geometry_1", type = "point", point = {0, 0, 0} },
    { id = "geometry_2", type = "point", point = {0, 2, 0} },
    { id = "geometry_3", type = "line", a = {0, 0, 0}, b = {3, 0, 0} },
    { id = "geometry_4", type = "line", a = {0, 0, 0}, b = {0, 3, 0} }
  },
  constraints = {
    { id = "constraint_1", type = "Coincident", participants = {"geometry_1", "geometry_3"} },
    { id = "constraint_2", type = "Perpendicular", participants = {"geometry_3", "geometry_4"} }
  }
}
