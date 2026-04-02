return {
  inputs = {
    { id = "input_length", value = 10.0, min = 1.0, max = 20.0, step = 0.5 },
    { id = "input_height", value = 3.0, min = 1.0, max = 8.0, step = 0.25 }
  },
  outputs = {
    { id = "output_span", value = 7.5 },
    { id = "output_ratio", value = 2.5 }
  },
  entities = {
    { id = "geometry_1", type = "point", point = {0, 0, 0} },
    { id = "geometry_2", type = "line", a = {0, 0, 0}, b = {10, 0, 0} }
  },
  constraints = {
    { id = "constraint_1", type = "Length", participants = {"geometry_2"}, value = 10.0, driven = false }
  }
}
