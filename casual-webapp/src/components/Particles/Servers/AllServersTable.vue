<template>
    <div>
        <!--form>
            <div class="form-group row">
                <label for="keywordInput" class="col-sm-2 col-form-label"><b>Search</b></label>
                <div class="col-sm-6">
                    <input class="form-control" id="keywordInput" type="text" ng-model="searchKeyword">
                </div>
            </div>
        </form-->
        <table class="table-style">
            <thead>
            <tr>
                <th scope="col">Alias</th>
                <th scope="col">Note</th>
                <th scope="col">Path</th>
                <th scope="col">Instances</th>
                <th scope="col">Memberships</th>
            </tr>
            </thead>
            <tbody>

            <tr v-for="server in servers" :key="server.alias">

                <td>
                    <router-link :to="{name: 'server', query: {alias: server.alias}}">
                        <a class="particle-link">{{server.alias || '-'}}</a>
                    </router-link>
                </td>
                <td>{{server.note}}</td>
                <td>{{server.path}}</td>
                <td>{{server.instances.length}}</td>
                <td>
                    <div v-for="group in server.membershipNames" :key="group">
                        <router-link :to="{name: 'group', query: {name: group}}">
                            <a>{{group}}</a>
                        </router-link>

                    </div>
                </td>

            </tr>

            </tbody>
        </table>

    </div>

</template>

<script>
    export default {
        name: "AllServersTable",
        props: ['servers'],
        data() {
            return {
                all: this.servers,
            }
        },
        computed: {
            alls: function () {
                return "";
            }
        },
        created() {
           // console.log(JSON.stringify(this.servers, null,1));
        }
    }

</script>

