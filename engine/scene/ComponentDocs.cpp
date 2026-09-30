#include "scene/ComponentDocs.hpp"

#include <format>
#include <vector>

namespace PimpEx::scene {

std::string components_markdown(const ComponentRegistry &registry) {
  std::string out = R"(# Komponenty

Aktor (GameObject) to nazwa + zestaw komponentów. Komponent to dane: jak aktor
wygląda, jak się porusza, co przechowuje. Zachowania dopisuje się w Lua
(`<Behavior>`, docs/behaviors.md) albo w `.swap` (`PROC ... FOR Komponent`).

```xml
<Actor name="Player" x="100" y="200" w="32" h="32" tags="hero">
  <Rect color="blue"/>
  <Collider/>
  <Health hp="5"/>                      <!-- komponent z components.xml -->
  <Behavior name="topdown_move" speed="4"/>
</Actor>
```

- Pola `Transform` i `Body` można pisać wprost na `<Actor>` (`x="10" vx="2"`).
- W `.swap` i Lua pole to `Aktor.pole` (skróty: `x`, `vx`, `rotation`...) albo
  `Aktor.Komponent.pole`, np. `Player.Health.hp`, `Cam.Camera.zoom`.
- Brakujący komponent zostaje dodany przy pierwszym `SET`.
- Ten plik powstaje z `pimpex components --markdown`. Dla projektu z
  `components.xml` ta komenda pokazuje też komponenty użytkownika.

## Komponenty użytkownika: `components.xml`

```xml
<Components>
  <Component name="Health" doc="Punkty życia">
    <Field name="hp" type="number" default="3"/>
    <Field name="shield" type="bool" default="false"/>
  </Component>
</Components>
```

Typy pól: `number`, `integer`, `bool`, `text`, `color`.
)";

  std::string group;
  for (const auto &c : registry.all()) {
    if (c.group != group) {
      group = c.group;
      out += std::format("\n## {}\n", group == "użytkownika" ? "Komponenty użytkownika (components.xml)"
                                                            : "Wbudowane: " + group);
    }
    out += std::format("\n### `<{}>`\n\n{}\n\n", c.name, c.doc.empty() ? "–" : c.doc);
    if (c.fields.empty()) {
      out += "Bez pól.\n";
      continue;
    }
    out += "| Pole | Typ | Domyślnie | Opis |\n|------|-----|-----------|------|\n";
    for (const auto &f : c.fields) {
      std::string type(field_type_name(f.type));
      if (!f.choices.empty()) {
        type.clear();
        for (const auto &choice : f.choices)
          type += (type.empty() ? "" : " \\| ") + choice;
      }
      auto def = field_value_to_string(f.defaultValue);
      if (f.type == FieldType::Bool)
        def = def == "1" ? "true" : "false";
      out += std::format("| `{}` | {} | `{}` | {}{} |\n", f.name, type, def.empty() ? "\"\"" : def,
                         f.doc, f.readOnly ? " (tylko do odczytu)" : "");
    }
  }
  return out;
}

} // namespace PimpEx::scene
