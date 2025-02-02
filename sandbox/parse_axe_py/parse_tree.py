import dataclasses
import pprint


@dataclasses.dataclass
class Node:
    name: str | None = None
    children: list["Node"] = dataclasses.field(default_factory=list)

    def render(self) -> str:
        if (self.name is None) and len(self.children) == 0:
            return 'CONCAT'
        else:
            assert self.name is not None, f'No name on node {pprint.pformat(self)}'
            retval = self.name
            if len(self.children) > 0:
                retval += '{'
                for child in self.children:
                    retval += ' '
                    retval += child.render()
                retval += ' }'
            return retval
